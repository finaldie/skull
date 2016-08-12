#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "flibs/fhash.h"
#include "flibs/flist.h"
#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_entity_mgr.h"

// Entity Manager
struct sk_entity_mgr_t {
    fhash* entity_mgr;
    flist* inactive_entities;
    flist* tmp_entities;
    struct sk_sched_t* owner_sched;
};

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
    int total_inactive      = 0;
    int entity_none         = 0;
    int entity_net          = 0;
    int entity_timer        = 0;
    int entity_ep           = 0;
    int entity_ep_timer     = 0;
    int entity_ep_txn_timer = 0;
    int entity_unknown      = 0;
    int cleaned             = 0;

    while (!flist_empty(mgr->inactive_entities)) {
        sk_entity_t* entity = flist_pop(mgr->inactive_entities);
        total_inactive++;

        int tag = sk_entity_tag(entity);
        switch (tag) {
        case SK_ENTITY_TAG_NONE:
            entity_none++;
            break;
        case SK_ENTITY_TAG_NET:
            entity_net++;
            break;
        case SK_ENTITY_TAG_TIMER:
            entity_timer++;
            break;
        case SK_ENTITY_TAG_EP:
            entity_ep++;
            break;
        case SK_ENTITY_TAG_EP_TIMER:
            entity_ep_timer++;
            break;
        case SK_ENTITY_TAG_EP_TXN_TIMER:
            entity_ep_txn_timer++;
            break;
        default:
            entity_unknown++;
            break;
        }

        // 1. Check whehter it can be destroyed
        int taskcnt = sk_entity_taskcnt(entity);
        if (taskcnt && !force) {
            //sk_entity_dump_txns(entity);
            //sk_print("entity(%p) taskcnt(%d) > 0, try to destroy it in next "
            //         "round\n", (void*)entity, taskcnt);
            flist_push(mgr->tmp_entities, entity);
            continue;
        }

        // 2. Update metrics
        sk_metrics_worker.entity_destroy.inc(1);
        if (SK_ENTITY_NET == sk_entity_type(entity)) {
            sk_metrics_worker.connection_destroy.inc(1);
        }

        // 3. Destroy entity totally
        sk_entity_t* deleted_entity =
            fhash_u64_del(mgr->entity_mgr, (uint64_t) (uintptr_t) entity);
        SK_ASSERT(deleted_entity == entity);
        sk_entity_destroy(entity);
        sk_print("clean up dead entity: %p\n", (void*)entity);
        cleaned++;
    }

    if (total_inactive) {
        sk_print("cleanup dead: total inactive: %d, cleaned: %d, entity_none: %d, "
                 "entity_net: %d, entity_timer: %d, entity_ep: %d, "
                 "entity_ep_timer: %d, entity_ep_txn_timer: %d, force: %d\n",
                 total_inactive, cleaned, entity_none, entity_net, entity_timer,
                 entity_ep, entity_ep_timer, entity_ep_txn_timer, force);
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
    if (SK_ENTITY_NET == sk_entity_type(entity)) {
        sk_metrics_worker.connection_create.inc(1);
    }
}

sk_entity_t* sk_entity_mgr_del(sk_entity_mgr_t* mgr, sk_entity_t* entity)
{
    if (!entity) {
        return NULL;
    }

    SK_ASSERT_MSG(sk_entity_owner(entity) == mgr, "entity: %p\n", (void*)entity);

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

