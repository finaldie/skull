#ifndef SK_ENTITY_H
#define SK_ENTITY_H

typedef enum sk_entity_status_t {
    SK_ENTITY_INIT = 0,   // an empty entity
    SK_ENTITY_ACTIVE,     // read to do the task
    SK_ENTITY_INACTIVE,   // ready to be deleted
    SK_ENTITY_DEAD        // already marked as deleted
} sk_entity_status_t;

struct sk_sched_t;
struct sk_entity_mgr_t;
struct sk_workflow_t;
typedef struct sk_entity_t sk_entity_t;

typedef struct sk_entity_opt_t {
    int  (*read)    (sk_entity_t*, void* buf, int len, void* ud);
    int  (*write)   (sk_entity_t*, const void* buf, int len, void* ud);
    void (*destroy) (sk_entity_t*, void* ud);
} sk_entity_opt_t;

// ENTITY
sk_entity_t* sk_entity_create(struct sk_sched_t* sched,
                              struct sk_workflow_t* workflow);
void sk_entity_destroy(sk_entity_t* entity);

int sk_entity_read(sk_entity_t* entity, void* buf, int buf_len);
int sk_entity_write(sk_entity_t* entity, const void* buf, int buf_len);

sk_entity_status_t sk_entity_status(sk_entity_t* entity);
struct sk_entity_mgr_t* sk_entity_owner(sk_entity_t* entity);
struct sk_sched_t* sk_entity_sched(sk_entity_t* entity);
struct sk_workflow_t* sk_entity_workflow(sk_entity_t* entity);

void sk_entity_setopt(sk_entity_t* entity, sk_entity_opt_t opt, void* ud);
void sk_entity_setowner(sk_entity_t* entity, struct sk_entity_mgr_t* mgr);
void sk_entity_mark(sk_entity_t* entity, sk_entity_status_t status);

// increase the query count
void sk_entity_inc_task_cnt(sk_entity_t* entity);

// decrease the query count
void sk_entity_dec_task_cnt(sk_entity_t* entity);

// get task cnt
int sk_entity_task_cnt(sk_entity_t* entity);

// create network entity from an empty entity
struct fev_buff;
void sk_net_entity_create(sk_entity_t* entity, struct fev_buff* evbuff);

#endif

