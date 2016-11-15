#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "flibs/fhash.h"
#include "flibs/flist.h"
#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_entity_mgr.h"

// return 0: continue iterating
// return non-zero: stop iterating
typedef int (*sk_entity_each_cb)(sk_entity_mgr_t* mgr,
                               sk_entity_t* entity,
                               void* ud);

void sk_entity_mgr_foreach(sk_entity_mgr_t* mgr,
                           sk_entity_each_cb each_cb,
                           void* ud);

// Entity Manager
struct sk_entity_mgr_t {
    fhash* entity_mgr;
    flist* inactive_entities;
    flist* tmp_entities;
    struct sk_sched_t* owner_sched;

    sk_entity_mgr_stat_t stat;
};

static
void _update_stat(sk_entity_mgr_t* mgr, const sk_entity_t* entity,
                  bool inactive, int value)
{
    if (!value) return;

    if (inactive) {
        mgr->stat.inactive += value;

        if (value > 0) {
            return;
        }
    }

    mgr->stat.total += value;

    sk_entity_type_t type = sk_entity_type(entity);
    switch (type) {
    case SK_ENTITY_NONE:
        mgr->stat.entity_none += value;
        break;
    case SK_ENTITY_SOCK_V4TCP:
        mgr->stat.entity_sock_v4tcp += value;
        break;
    case SK_ENTITY_SOCK_V4UDP:
        mgr->stat.entity_sock_v4udp += value;
        break;
    case SK_ENTITY_TIMER:
        mgr->stat.entity_timer += value;
        break;
    case SK_ENTITY_EP_V4TCP:
        mgr->stat.entity_ep_v4tcp += value;
        break;
    case SK_ENTITY_EP_V4UDP:
        mgr->stat.entity_ep_v4udp += value;
        break;
    case SK_ENTITY_EP_TIMER:
        mgr->stat.entity_ep_timer += value;
        break;
    case SK_ENTITY_EP_TXN_TIMER:
        mgr->stat.entity_ep_txn_timer++;
        break;
    default:
        SK_ASSERT_MSG(0, "Unknown entity type: %d\n", type);
        break;
    }
}

static
int _mark_dead(sk_entity_mgr_t* mgr, sk_entity_t* entity, void* ud)
{
    sk_print("mark entity as dead, entity: %p\n", (void*)entity);
    sk_entity_mgr_del(mgr, entity);
    return 0;
}

static
void _cleanup_dead_entities(sk_entity_mgr_t* mgr, int force)
{
    while (!flist_empty(mgr->inactive_entities)) {
        sk_entity_t* entity = flist_pop(mgr->inactive_entities);

        // 1. Check whehter it can be destroyed
        int taskcnt = sk_entity_taskcnt(entity);
        if (taskcnt && !force) {
            //sk_entity_dump_txns(entity);
            //sk_print("entity(%p) taskcnt(%d) > 0, try to destroy it in next "
            //         "round\n", (void*)entity, taskcnt);

            // Still has task incompleted, push back and waiting for next round
            flist_push(mgr->tmp_entities, entity);
            continue;
        }

        // 2. Update metrics
        sk_metrics_worker.entity_destroy.inc(1);
        if (SK_ENTITY_SOCK_V4TCP == sk_entity_type(entity)) {
            sk_metrics_worker.connection_destroy.inc(1);
        }

        // 3. Destroy entity totally
        _update_stat(mgr, entity, true, -1);

        sk_entity_t* deleted_entity =
            fhash_u64_del(mgr->entity_mgr, (uint64_t) (uintptr_t) entity);
        SK_ASSERT(deleted_entity == entity);
        sk_entity_destroy(entity);
        sk_print("clean up dead entity: %p\n", (void*)entity);
    }

    // Swap inactive queue and temp queue
    flist* tmp = mgr->inactive_entities;
    mgr->inactive_entities = mgr->tmp_entities;
    mgr->tmp_entities = tmp;

    SK_ASSERT(flist_empty(mgr->tmp_entities));
}

// APIs
sk_entity_mgr_t* sk_entity_mgr_create(uint32_t idx_sz)
{
    sk_entity_mgr_t* mgr = calloc(1, sizeof(*mgr));
    mgr->entity_mgr        = fhash_u64_create(idx_sz, FHASH_MASK_AUTO_REHASH);
    mgr->inactive_entities = flist_create();
    mgr->tmp_entities      = flist_create();
    return mgr;
}

void sk_entity_mgr_destroy(sk_entity_mgr_t* mgr)
{
    // Mark all activity entity to inactive, then clean up
    sk_entity_mgr_foreach(mgr, _mark_dead, NULL);
    _cleanup_dead_entities(mgr, 1);
    flist_delete(mgr->inactive_entities);
    flist_delete(mgr->tmp_entities);

    fhash_u64_delete(mgr->entity_mgr);
    free(mgr);
}

void sk_entity_mgr_add(sk_entity_mgr_t* mgr, sk_entity_t* entity)
{
    fhash_u64_set(mgr->entity_mgr, (uint64_t) (uintptr_t) entity, entity);
    sk_entity_setowner(entity, mgr);

    // Recored metrics
    sk_metrics_worker.entity_create.inc(1);
    if (SK_ENTITY_SOCK_V4TCP == sk_entity_type(entity)) {
        sk_metrics_worker.connection_create.inc(1);
    }

    _update_stat(mgr, entity, false, 1);
}

sk_entity_t* sk_entity_mgr_del(sk_entity_mgr_t* mgr, sk_entity_t* entity)
{
    if (!entity) {
        return NULL;
    }

    SK_ASSERT_MSG(sk_entity_owner(entity) == mgr,
                  "Incorrect owner of this entity: %p\n", (void*)entity);

    if (sk_entity_status(entity) == SK_ENTITY_DEAD) {
        // already dead, it will be destroy totally when the clean_dead be
        // called
        return entity;
    }

    // mark it to inactive and destroy the internal data first
    sk_entity_mark(entity, SK_ENTITY_INACTIVE);
    sk_entity_destroy(entity);

    // mark it to dead and wait for being freed totally
    sk_entity_mark(entity, SK_ENTITY_DEAD);
    int ret = flist_push(mgr->inactive_entities, entity);
    SK_ASSERT(!ret);

    _update_stat(mgr, entity, true, 1);
    return entity;
}

void sk_entity_mgr_foreach(sk_entity_mgr_t* mgr,
                           sk_entity_each_cb each_cb,
                           void* ud)
{
    fhash_u64_iter iter = fhash_u64_iter_new(mgr->entity_mgr);

    void* data = NULL;
    while ((data = fhash_u64_next(&iter))) {
        if (each_cb(mgr, iter.value, ud)) {
            break;
        }
    }

    fhash_u64_iter_release(&iter);
}

void sk_entity_mgr_clean_dead(sk_entity_mgr_t* mgr)
{
    _cleanup_dead_entities(mgr, 0);
}

struct sk_sched_t* sk_entity_mgr_sched(const sk_entity_mgr_t* mgr)
{
    return mgr->owner_sched;
}

void sk_entity_mgr_setsched(sk_entity_mgr_t* mgr, struct sk_sched_t* owner_sched)
{
    SK_ASSERT(!mgr->owner_sched);
    mgr->owner_sched = owner_sched;
}

sk_entity_mgr_stat_t sk_entity_mgr_stat(const sk_entity_mgr_t* mgr)
{
    return mgr->stat;
}
