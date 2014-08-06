#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api/sk_entity.h"

struct sk_entity_t {
    struct sk_entity_mgr_t* owner;
    struct sk_sched_t* sched;
    sk_entity_opt_t opt;

    int   fd;
    int   status;
    void* ud;
};

sk_entity_t* sk_entity_create(int fd, struct sk_sched_t* sched)
{
    sk_entity_t* entity = calloc(1, sizeof(*entity));
    entity->owner = NULL;
    entity->sched = sched;
    entity->fd = fd;
    entity->status = SK_ENTITY_ACTIVE;
    return entity;
}

void sk_entity_setopt(sk_entity_t* entity, sk_entity_opt_t opt, void* ud)
{
    entity->opt = opt;
    entity->ud = ud;
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

void sk_entity_set_owner(sk_entity_t* entity, struct sk_entity_mgr_t* mgr)
{
    entity->owner = mgr;
}

// set the status as inactive, which means this entity is ready to be destroyed
void sk_entity_mark_inactive(sk_entity_t* entity)
{
    entity->status = SK_ENTITY_INACTIVE;
}

void sk_entity_mark_dead(sk_entity_t* entity)
{
    entity->status = SK_ENTITY_DEAD;
}

struct sk_entity_mgr_t* sk_entity_owner(sk_entity_t* entity)
{
    return entity->owner;
}

struct sk_sched_t* sk_entity_sched(sk_entity_t* entity)
{
    return entity->sched;
}

int sk_entity_fd(sk_entity_t* entity)
{
    return entity->fd;
}

int sk_entity_status(sk_entity_t* entity)
{
    return entity->status;
}
