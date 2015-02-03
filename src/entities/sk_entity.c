#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api/sk_entity.h"
#include "api/sk_workflow.h"
#include "api/sk_txn.h"

struct sk_entity_t {
    struct sk_entity_mgr_t* owner;
    sk_workflow_t*          workflow;
    sk_txn_t*               txn;      // used for storing the inactive txn
    sk_entity_opt_t         opt;

    sk_entity_status_t status;
    int   task_cnt;
    void* ud;
};

sk_entity_t* sk_entity_create(sk_workflow_t* workflow)
{
    sk_entity_t* entity = calloc(1, sizeof(*entity));
    entity->owner = NULL;
    entity->workflow = workflow;
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
    free(entity);
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

void sk_entity_settxn(sk_entity_t* entity, sk_txn_t* txn)
{
    entity->txn = txn;
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

sk_txn_t* sk_entity_txn(sk_entity_t* entity)
{
    return entity->txn;
}

sk_entity_status_t sk_entity_status(sk_entity_t* entity)
{
    return entity->status;
}

// increase the query count
void sk_entity_inc_task_cnt(sk_entity_t* entity)
{
    entity->task_cnt++;
}

// decrease the query count
void sk_entity_dec_task_cnt(sk_entity_t* entity)
{
    entity->task_cnt--;
}

// get task cnt
int sk_entity_task_cnt(sk_entity_t* entity)
{
    return entity->task_cnt;
}
