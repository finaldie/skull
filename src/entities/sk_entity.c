#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "flibs/fhash.h"
#include "api/sk_utils.h"
#include "api/sk_entity.h"
#include "api/sk_workflow.h"
#include "api/sk_metrics.h"
#include "api/sk_txn.h"
#include "api/sk_sched.h"

struct sk_entity_t {
    struct sk_entity_mgr_t* owner;
    sk_sched_t*             owner_sched;
    sk_workflow_t*          workflow;
    sk_txn_t*               half_txn; // store the incompleted txn
    sk_entity_opt_t         opt;
    fhash*                  txns;

    sk_entity_status_t status;
    int   task_cnt;
    void* ud;
};

// increase the query count
static
void _entity_taskcnt_inc(sk_entity_t* entity)
{
    entity->task_cnt++;
}

// decrease the query count
static
void _entity_taskcnt_dec(sk_entity_t* entity)
{
    entity->task_cnt--;
}

// default opts
static
ssize_t _default_read(sk_entity_t* entity, void* buf, size_t buf_len, void* ud)
{
    errno = ENOSYS;
    return -1;
}

static
ssize_t _default_write(sk_entity_t* entity, const void* buf, size_t buf_len,
                       void* ud)
{
    errno = ENOSYS;
    return -1;
}

static
void _default_destroy(sk_entity_t* entity, void* ud)
{
    return;
}

sk_entity_opt_t default_entity_opt = {
    .read    = _default_read,
    .write   = _default_write,
    .destroy = _default_destroy
};

// APIs
sk_entity_t* sk_entity_create(sk_workflow_t* workflow)
{
    sk_entity_t* entity = calloc(1, sizeof(*entity));
    entity->owner    = NULL;
    entity->workflow = workflow;
    entity->opt      = default_entity_opt;
    entity->txns     = fhash_u64_create(0, FHASH_MASK_AUTO_REHASH);
    entity->status   = SK_ENTITY_ACTIVE;

    sk_metrics_global.entity_create.inc(1);
    sk_print("create entity %p\n", (void*)entity);
    return entity;
}

void sk_entity_setopt(sk_entity_t* entity, sk_entity_opt_t opt, void* ud)
{
    entity->opt.read  = opt.read  ? opt.read  : default_entity_opt.read;
    entity->opt.write = opt.write ? opt.write : default_entity_opt.write;
    entity->opt.destroy = opt.destroy ? opt.destroy : default_entity_opt.destroy;

    entity->ud = ud;
}

void sk_entity_destroy(sk_entity_t* entity)
{
    if (!entity) return;

    sk_entity_status_t status = entity->status;
    SK_ASSERT(status == SK_ENTITY_INACTIVE || status == SK_ENTITY_DEAD);

    if (entity->status == SK_ENTITY_INACTIVE) {
        // Release resources
        entity->opt.destroy(entity, entity->ud);
        entity->ud = NULL;
    } else {
        // Update metrcis
        sk_metrics_global.entity_destroy.inc(1);

        // clean up all txns
        fhash_u64_iter iter = fhash_u64_iter_new(entity->txns);
        sk_txn_t* txn = NULL;
        while ((txn = fhash_u64_next(&iter))) {
            sk_print("clean up the unfinished txn\n");
            sk_txn_destroy(txn);
        }
        fhash_u64_iter_release(&iter);
        fhash_u64_delete(entity->txns);

        free(entity);
    }
}

ssize_t sk_entity_read(sk_entity_t* entity, void* buf, size_t buf_len)
{
    return entity->opt.read(entity, buf, buf_len, entity->ud);
}

ssize_t sk_entity_write(sk_entity_t* entity, const void* buf, size_t buf_len)
{
    return entity->opt.write(entity, buf, buf_len, entity->ud);
}

void sk_entity_setowner(sk_entity_t* entity, struct sk_entity_mgr_t* mgr)
{
    SK_ASSERT(!entity->owner);
    entity->owner = mgr;
}

void sk_entity_sethalftxn(sk_entity_t* entity, sk_txn_t* half_txn)
{
    entity->half_txn = half_txn;
}

void sk_entity_mark(sk_entity_t* entity, sk_entity_status_t status)
{
    entity->status = status;
}

struct sk_entity_mgr_t* sk_entity_owner(sk_entity_t* entity)
{
    return entity->owner;
}

sk_workflow_t* sk_entity_workflow(sk_entity_t* entity)
{
    return entity->workflow;
}

sk_txn_t* sk_entity_halftxn(sk_entity_t* entity)
{
    return entity->half_txn;
}

sk_entity_status_t sk_entity_status(sk_entity_t* entity)
{
    return entity->status;
}

void sk_entity_txnadd(sk_entity_t* entity, struct sk_txn_t* txn)
{
    fhash_u64_set(entity->txns, (uint64_t) (uintptr_t) txn, txn);
    _entity_taskcnt_inc(entity);
}

void sk_entity_txndel(sk_entity_t* entity, struct sk_txn_t* txn)
{
    sk_txn_t* deleted = fhash_u64_del(entity->txns, (uint64_t) (uintptr_t) txn);
    SK_ASSERT(deleted == txn);
    _entity_taskcnt_dec(entity);
}

// get task cnt
int sk_entity_taskcnt(sk_entity_t* entity)
{
    return entity->task_cnt;
}

extern sk_entity_opt_t sk_entity_stdin_opt;
extern sk_entity_opt_t sk_entity_net_opt;

sk_entity_type_t sk_entity_type(sk_entity_t* entity)
{
    if (entity->opt.read == sk_entity_net_opt.read) {
        return SK_ENTITY_NET;
    } else if (entity->opt.read == sk_entity_stdin_opt.read) {
        return SK_ENTITY_STD;
    } else {
        return SK_ENTITY_NONE;
    }
}
