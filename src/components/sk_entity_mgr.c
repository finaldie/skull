#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fhash/fhash.h"
#include "flist/flist.h"
#include "api/sk_assert.h"
#include "api/sk_entity_mgr.h"

// Entity Manager
struct sk_entity_mgr_t {
    struct sk_sched_t* owner;
    fhash* entity_mgr;
    flist* inactive_entitys;
};

sk_entity_mgr_t* sk_entity_mgr_create(struct sk_sched_t* owner, size_t size)
{
    sk_entity_mgr_t* mgr = malloc(sizeof(*mgr));
    mgr->owner = owner;
    mgr->entity_mgr = fhash_int_create(size, FHASH_MASK_AUTO_REHASH);
    mgr->inactive_entitys = flist_create();
    return mgr;
}

void sk_entity_mgr_destroy(sk_entity_mgr_t* mgr)
{
    fhash_int_delete(mgr->entity_mgr);
    flist_delete(mgr->inactive_entitys);
    free(mgr);
}

struct sk_sched_t* sk_entity_mgr_owner(sk_entity_mgr_t* mgr)
{
    return mgr->owner;
}

sk_entity_t* sk_entity_mgr_get(sk_entity_mgr_t* mgr, int fd)
{
    return fhash_int_get(mgr->entity_mgr, fd);
}

void sk_entity_mgr_add(sk_entity_mgr_t* mgr, int fd, sk_entity_t* entity)
{
    fhash_int_set(mgr->entity_mgr, fd, entity);
    sk_entity_set_owner(entity, mgr);
}

sk_entity_t* sk_entity_mgr_del(sk_entity_mgr_t* mgr, int fd)
{
    sk_entity_t* entity = sk_entity_mgr_get(mgr, fd);
    if (!entity) {
        return NULL;
    }

    if (sk_entity_status(entity) == SK_ENTITY_DEAD) {
        // already dead, it will be destroy totally when the clean_dead be
        // called
        return entity;
    }

    sk_entity_mark_dead(entity);

    int ret = flist_push(mgr->inactive_entitys, entity);
    SK_ASSERT(!ret);

    return entity;
}

void sk_entity_mgr_foreach(sk_entity_mgr_t* mgr,
                           sk_entity_each_cb each_cb,
                           void* ud)
{
    fhash_int_iter iter = fhash_int_iter_new(mgr->entity_mgr);

    void* data = NULL;
    while ((data = fhash_int_next(&iter))) {
        if (each_cb(mgr, iter.value, ud)) {
            break;
        }
    }

    fhash_int_iter_release(&iter);
}

void sk_entity_mgr_clean_dead(sk_entity_mgr_t* mgr)
{
    while (!flist_empty(mgr->inactive_entitys)) {
        sk_entity_t* entity = flist_pop(mgr->inactive_entitys);

        sk_entity_t* deleted_entity = fhash_int_del(mgr->entity_mgr,
                                                    sk_entity_fd(entity));
        SK_ASSERT(deleted_entity == entity);
        sk_entity_destroy(entity);
    }
}
