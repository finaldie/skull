#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fhash/fhash.h"
#include "flist/flist.h"
#include "api/sk_assert.h"
#include "api/sk_entity_mgr.h"

struct sk_entity_t {
    struct sk_entity_mgr_t* owner;
    sk_entity_opt_t opt;

    int   fd;
    int   status;
    void* ud;
};

sk_entity_t* sk_entity_create(int fd, void* ud, sk_entity_opt_t opt)
{
    sk_entity_t* entity = malloc(sizeof(*entity));
    memset(entity, 0, sizeof(*entity));
    entity->owner = NULL;
    entity->opt = opt;
    entity->fd = fd;
    entity->status = SK_ENTITY_ACTIVE;
    entity->ud = ud;
    return entity;
}

void sk_entity_destroy(sk_entity_t* entity)
{
    entity->opt.destroy(entity, entity->ud);
    close(entity->fd);
    free(entity);
}

int sk_entity_read(sk_entity_t* entity, void* buf, int buf_len)
{
    return entity->opt.read(entity, buf, buf_len, entity->ud);
}

int sk_entity_write(sk_entity_t* entity, const void* buf, int buf_len)
{
    return entity->opt.write(entity, buf, buf_len, entity->ud);
}

sk_entity_mgr_t* sk_entity_owner(sk_entity_t* entity)
{
    return entity->owner;
}

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

void sk_entity_mgr_add(sk_entity_mgr_t* mgr, sk_entity_t* entity)
{
    fhash_int_set(mgr->entity_mgr, entity->fd, entity);
    entity->owner = mgr;
}

sk_entity_t* sk_entity_mgr_del(sk_entity_mgr_t* mgr, sk_entity_t* entity)
{
    if (entity->status == SK_ENTITY_DEAD) {
        // already dead, it will be destroy totally when the clean_dead be
        // called
        return entity;
    }

    entity->status = SK_ENTITY_DEAD;
    sk_entity_t* deleted_entity = fhash_int_del(mgr->entity_mgr, entity->fd);
    SK_ASSERT(deleted_entity == entity);

    int ret = flist_push(mgr->inactive_entitys, deleted_entity);
    SK_ASSERT(!ret);

    return deleted_entity;
}

void sk_entity_mgr_foreach(sk_entity_mgr_t* mgr, sk_entity_each_cb each_cb, void* ud)
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
        sk_entity_destroy(entity);
    }
}
