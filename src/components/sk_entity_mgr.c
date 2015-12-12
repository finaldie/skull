#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "flibs/fhash.h"
#include "flibs/flist.h"
#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_entity_mgr.h"

static
int _mark_dead(sk_entity_mgr_t* mgr, sk_entity_t* entity, void* ud)
{
    sk_entity_mgr_del(mgr, entity);
    return 0;
}

// Entity Manager
struct sk_entity_mgr_t {
    fhash* entity_mgr;
    flist* inactive_entities;
};

sk_entity_mgr_t* sk_entity_mgr_create(uint32_t idx_sz)
{
    sk_entity_mgr_t* mgr = malloc(sizeof(*mgr));
    mgr->entity_mgr = fhash_u64_create(idx_sz, FHASH_MASK_AUTO_REHASH);
    mgr->inactive_entities = flist_create();
    return mgr;
}

void sk_entity_mgr_destroy(sk_entity_mgr_t* mgr)
{
    // Mark all activity entity to inactive, then clean up
    sk_entity_mgr_foreach(mgr, _mark_dead, NULL);
    sk_entity_mgr_clean_dead(mgr);
    flist_delete(mgr->inactive_entities);

    fhash_u64_delete(mgr->entity_mgr);
    free(mgr);
}

void sk_entity_mgr_add(sk_entity_mgr_t* mgr, sk_entity_t* entity)
{
    fhash_u64_set(mgr->entity_mgr, (uint64_t)entity, entity);
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

    if (sk_entity_status(entity) == SK_ENTITY_DEAD) {
        // already dead, it will be destroy totally when the clean_dead be
        // called
        return entity;
    }

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
    while (!flist_empty(mgr->inactive_entities)) {
        sk_entity_t* entity = flist_pop(mgr->inactive_entities);

        sk_entity_t* deleted_entity = fhash_u64_del(mgr->entity_mgr,
                                                    (uint64_t)entity);
        SK_ASSERT(deleted_entity == entity);
        sk_entity_destroy(entity);
        sk_print("clean up dead entity\n");

        // Update metrics
        sk_metrics_worker.entity_destroy.inc(1);
        if (SK_ENTITY_NET == sk_entity_type(entity)) {
            sk_metrics_worker.connection_destroy.inc(1);
        }
    }
}
