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
    fhash*                  timers;
    sk_entity_type_t        type;
    sk_entity_status_t      status;
    int                     flags;
    int                     task_cnt;
    void*                   ud;
};

// increase the query count
static inline
void _entity_taskcnt_inc(sk_entity_t* entity)
{
    entity->task_cnt++;
}

// decrease the query count
static inline
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

static
void* _default_rbufget(const sk_entity_t* entity, void* ud)
{
    errno = ENOSYS;
    return NULL;
}

static
size_t _default_rbufsz(const sk_entity_t* entity, void* ud)
{
    errno = ENOSYS;
    return 0;
}

static
size_t _default_rbufpop(sk_entity_t* entity, size_t popsz, void* ud)
{
    errno = ENOSYS;
    return 0;
}

static
int _default_peer(const sk_entity_t* entity, sk_entity_peer_t* peer, void* ud)
{
    if (!peer) return 0;
    memset(peer, 0, sizeof(*peer));

    peer->port   = -1;
    peer->family = -1;
    strncpy(peer->name, "Unknown", INET6_ADDRSTRLEN);
    return 0;
}

sk_entity_opt_t default_opt = {
    .read    = _default_read,
    .write   = _default_write,
    .destroy = _default_destroy,
    .rbufget = _default_rbufget,
    .rbufsz  = _default_rbufsz,
    .rbufpop = _default_rbufpop,
    .peer    = _default_peer
};

// APIs
sk_entity_t* sk_entity_create(sk_workflow_t* workflow, sk_entity_type_t type)
{
    sk_entity_t* entity = calloc(1, sizeof(*entity));
    entity->owner    = NULL;
    entity->workflow = workflow;
    entity->opt      = default_opt;
    entity->txns     = fhash_u64_create(0, FHASH_MASK_AUTO_REHASH);
    entity->timers   = fhash_u64_create(0, FHASH_MASK_AUTO_REHASH);
    entity->status   = SK_ENTITY_ACTIVE;
    entity->type     = type;

    sk_metrics_global.entity_create.inc(1);
    sk_print("create entity %p\n", (void*)entity);
    return entity;
}

void sk_entity_setopt(sk_entity_t* entity, sk_entity_opt_t opt, void* ud)
{
    entity->opt.read  = opt.read  ? opt.read  : default_opt.read;
    entity->opt.write = opt.write ? opt.write : default_opt.write;
    entity->opt.destroy = opt.destroy ? opt.destroy : default_opt.destroy;
    entity->opt.rbufget = opt.rbufget ? opt.rbufget : default_opt.rbufget;
    entity->opt.rbufsz  = opt.rbufsz  ? opt.rbufsz  : default_opt.rbufsz;
    entity->opt.rbufpop = opt.rbufpop ? opt.rbufpop : default_opt.rbufpop;
    entity->opt.peer    = opt.peer ? opt.peer : default_opt.peer;

    entity->ud = ud;
}

// Debugging Api
void sk_entity_dump_txns(const sk_entity_t* entity)
{
    fhash_u64_iter iter = fhash_u64_iter_new(entity->txns);
    sk_txn_t* txn = NULL;
    while ((txn = fhash_u64_next(&iter))) {
        sk_print("entity txns: state: %d, txn log: %s\n", sk_txn_state(txn), sk_txn_log(txn));
        sk_txn_dump_tasks(txn);
    }
    fhash_u64_iter_release(&iter);
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
        // 1. Update metrcis
        sk_metrics_global.entity_destroy.inc(1);

        // 2. Clean up all txns
        fhash_u64_iter iter = fhash_u64_iter_new(entity->txns);
        sk_txn_t* txn = NULL;
        while ((txn = fhash_u64_next(&iter))) {
            sk_print("clean up the unfinished txn\n");
            sk_txn_destroy(txn);
        }
        fhash_u64_iter_release(&iter);
        fhash_u64_delete(entity->txns);

        // 3. Clean up all timers' data
        fhash_u64_iter titer = fhash_u64_iter_new(entity->timers);
        sk_obj_t* obj = NULL;
        while ((obj = fhash_u64_next(&titer))) {
            sk_print("clean up the unfinished timer data\n");
            sk_obj_destroy(obj);
        }
        fhash_u64_iter_release(&titer);
        fhash_u64_delete(entity->timers);

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

void*   sk_entity_rbufget(const sk_entity_t* entity)
{
    return entity->opt.rbufget(entity, entity->ud);
}

size_t  sk_entity_rbufsz(const sk_entity_t* entity)
{
    return entity->opt.rbufsz(entity, entity->ud);
}

size_t  sk_entity_rbufpop(sk_entity_t* entity, size_t popsz)
{
    return entity->opt.rbufpop(entity, popsz, entity->ud);
}

int sk_entity_peer(const sk_entity_t* entity, sk_entity_peer_t* peer)
{
    if (!peer) return 1;
    return entity->opt.peer(entity, peer, entity->ud);
}

sk_entity_type_t sk_entity_type(const sk_entity_t* entity)
{
    return entity->type;
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

void sk_entity_setflags(sk_entity_t* entity, int flags) {
    entity->flags = flags;
}

int  sk_entity_flags(const sk_entity_t* entity) {
    return entity->flags;
}

struct sk_entity_mgr_t* sk_entity_owner(const sk_entity_t* entity)
{
    return entity->owner;
}

sk_workflow_t* sk_entity_workflow(const sk_entity_t* entity)
{
    return entity->workflow;
}

sk_txn_t* sk_entity_halftxn(const sk_entity_t* entity)
{
    return entity->half_txn;
}

sk_entity_status_t sk_entity_status(const sk_entity_t* entity)
{
    return entity->status;
}

void sk_entity_txnadd(sk_entity_t* entity, const struct sk_txn_t* txn)
{
    fhash_u64_set(entity->txns, (uint64_t) (uintptr_t) txn, txn);
    _entity_taskcnt_inc(entity);
}

void sk_entity_txndel(sk_entity_t* entity, const struct sk_txn_t* txn)
{
    sk_txn_t* deleted = fhash_u64_del(entity->txns, (uint64_t) (uintptr_t) txn);
    SK_ASSERT(deleted == txn);
    _entity_taskcnt_dec(entity);
}

// get task cnt
int sk_entity_taskcnt(const sk_entity_t* entity)
{
    return entity->task_cnt;
}

void sk_entity_timeradd(sk_entity_t* entity, const sk_obj_t* obj)
{
    fhash_u64_set(entity->timers, (uint64_t) (uintptr_t) obj, obj);
}

void sk_entity_timerdel(sk_entity_t* entity, const sk_obj_t* obj)
{
    sk_obj_t* deleted = fhash_u64_del(entity->timers, (uint64_t)(uintptr_t)obj);
    SK_ASSERT(deleted == obj);
}

