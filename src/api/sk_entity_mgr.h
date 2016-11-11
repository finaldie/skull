#ifndef SK_ENTITY_MGR_H
#define SK_ENTITY_MGR_H

#include <stddef.h>
#include <stdint.h>

#include "api/sk_entity.h"

struct sk_sched_t;
typedef struct sk_entity_mgr_t sk_entity_mgr_t;

sk_entity_mgr_t* sk_entity_mgr_create(uint32_t size);
void sk_entity_mgr_destroy(sk_entity_mgr_t* mgr);

void sk_entity_mgr_add(sk_entity_mgr_t* mgr, sk_entity_t* entity);
sk_entity_t* sk_entity_mgr_del(sk_entity_mgr_t* mgr, sk_entity_t* entity);
void sk_entity_mgr_clean_dead(sk_entity_mgr_t* mgr);

struct sk_sched_t* sk_entity_mgr_sched(const sk_entity_mgr_t* mgr);
void sk_entity_mgr_setsched(sk_entity_mgr_t* mgr,
                            struct sk_sched_t* owner_sched);

typedef struct sk_entity_mgr_stat_t {
    int total;
    int inactive;

    int entity_none;
    int entity_sock_v4tcp;  // Client entity, v4 tcp
    int entity_sock_v4udp;  // Client entity, v4 udp
    int entity_timer;
    int entity_ep_v4tcp;
    int entity_ep_v4udp;
    int entity_ep_timer;
    int entity_ep_txn_timer;
} sk_entity_mgr_stat_t;

sk_entity_mgr_stat_t sk_entity_mgr_stat(const sk_entity_mgr_t*);

#endif

