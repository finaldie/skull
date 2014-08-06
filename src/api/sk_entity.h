#ifndef SK_ENTITY_H
#define SK_ENTITY_H

#define SK_ENTITY_ACTIVE   0
#define SK_ENTITY_INACTIVE 1
#define SK_ENTITY_DEAD     2

struct sk_sched_t;
struct sk_entity_mgr_t;
typedef struct sk_entity_t sk_entity_t;

typedef struct sk_entity_opt_t {
    int  (*read)    (sk_entity_t*, void* buf, int len, void* ud);
    int  (*write)   (sk_entity_t*, const void* buf, int len, void* ud);
    void (*destroy) (sk_entity_t*, void* ud);
} sk_entity_opt_t;

// ENTITY
sk_entity_t* sk_entity_create(int fd, struct sk_sched_t* sched);
void sk_entity_destroy(sk_entity_t* entity);

int sk_entity_read(sk_entity_t* entity, void* buf, int buf_len);
int sk_entity_write(sk_entity_t* entity, const void* buf, int buf_len);

int sk_entity_fd(sk_entity_t* entity);
int sk_entity_status(sk_entity_t* entity);
struct sk_entity_mgr_t* sk_entity_owner(sk_entity_t* entity);
struct sk_sched_t* sk_entity_sched(sk_entity_t* entity);

void sk_entity_setopt(sk_entity_t* entity, sk_entity_opt_t opt, void* ud);
void sk_entity_set_owner(sk_entity_t* entity, struct sk_entity_mgr_t* mgr);
void sk_entity_mark_inactive(sk_entity_t* entity);
void sk_entity_mark_dead(sk_entity_t* entity);

// create network entity
struct fev_buff;
void sk_net_entity_create(sk_entity_t* entity, struct fev_buff* evbuff);

#endif

