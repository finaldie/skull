#ifndef SK_ENTITY_MGR_H
#define SK_ENTITY_MGR_H

#include <stddef.h>

#define SK_ENTITY_ACTIVE 0
#define SK_ENTITY_DEAD   1

struct sk_sched_t;
typedef struct sk_entity_t sk_entity_t;
typedef struct sk_entity_mgr_t sk_entity_mgr_t;

typedef struct sk_entity_opt_t {
    void (*init)    (sk_entity_t*, struct sk_sched_t*, void* ud);
    int  (*read)    (sk_entity_t*, void* buf, int len, void* ud);
    int  (*write)   (sk_entity_t*, const void* buf, int len, void* ud);
    void (*error)   (sk_entity_t*, void* ud);
    void (*destroy) (sk_entity_t*, void* ud);
} sk_entity_opt_t;


// ENTITY
sk_entity_t* sk_entity_create(int fd, void* ud, sk_entity_opt_t opt);
void sk_entity_destroy(sk_entity_t* entity);
int sk_entity_read(sk_entity_t* entity, void* buf, int buf_len);
int sk_entity_write(sk_entity_t* entity, const void* buf, int buf_len);
sk_entity_mgr_t* sk_entity_owner(sk_entity_t* entity);


// ENTITY MANAGER
sk_entity_mgr_t* sk_entity_mgr_create(struct sk_sched_t* owner, size_t size);
void sk_entity_mgr_destroy(sk_entity_mgr_t* mgr);
struct sk_sched_t* sk_entity_mgr_owner(sk_entity_mgr_t* mgr);
void sk_entity_mgr_add(sk_entity_mgr_t* mgr, sk_entity_t* entity);
sk_entity_t* sk_entity_mgr_del(sk_entity_mgr_t* mgr, sk_entity_t* entity);
void sk_entity_mgr_clean_dead(sk_entity_mgr_t* mgr);

// return 0: continue iterating
// return non-zero: stop iterating
typedef int (*sk_entity_each_cb)(sk_entity_mgr_t* mgr,
                               sk_entity_t* entity,
                               void* ud);

void sk_entity_mgr_foreach(sk_entity_mgr_t* mgr,
                           sk_entity_each_cb each_cb,
                           void* ud);

#endif

