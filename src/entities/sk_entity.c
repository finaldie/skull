#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "api/sk_entity.h"
#include "api/sk_workflow.h"
#include "api/sk_metrics.h"
#include "api/sk_txn.h"

struct sk_entity_t {
    struct sk_entity_mgr_t* owner;
    sk_workflow_t*          workflow;
    sk_txn_t*               half_txn; // store the incompleted txn
    sk_entity_opt_t         opt;

    sk_entity_status_t status;
    int   task_cnt;
    void* ud;
};

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
    entity->status   = SK_ENTITY_ACTIVE;

    sk_metrics_global.entity_create.inc(1);
    return entity;
}

void sk_entity_setopt(sk_entity_t* entity, sk_entity_opt_t opt, void* ud)
{
    entity->opt.read  = opt.read  ? opt.read  : default_entity_opt.read;
    entity->opt.write = opt.write ? opt.write : default_entity_opt.write;
    entity->opt.destroy = opt.destroy ? opt.destroy : default_entity_opt.destroy;

    entity->ud = ud;

    // Update metrics
    if (SK_ENTITY_NET == sk_entity_type(entity)) {
        sk_metrics_global.connection_create.inc(1);
    }
}

void sk_entity_destroy(sk_entity_t* entity)
{
    entity->opt.destroy(entity, entity->ud);
    free(entity);

    // Update metrcis
    sk_metrics_global.entity_destroy.inc(1);
    if (SK_ENTITY_NET == sk_entity_type(entity)) {
        sk_metrics_global.connection_destroy.inc(1);
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

// increase the query count
void sk_entity_taskcnt_inc(sk_entity_t* entity)
{
    entity->task_cnt++;
}

// decrease the query count
void sk_entity_taskcnt_dec(sk_entity_t* entity)
{
    entity->task_cnt--;
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
