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

struct sk_sched_t* sk_entity_mgr_sched(sk_entity_mgr_t* mgr);
void sk_entity_mgr_setsched(sk_entity_mgr_t* mgr,
                            struct sk_sched_t* owner_sched);

// return 0: continue iterating
// return non-zero: stop iterating
typedef int (*sk_entity_each_cb)(sk_entity_mgr_t* mgr,
                               sk_entity_t* entity,
                               void* ud);

void sk_entity_mgr_foreach(sk_entity_mgr_t* mgr,
                           sk_entity_each_cb each_cb,
                           void* ud);

#endif

