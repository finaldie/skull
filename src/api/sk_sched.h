#ifndef SK_SCHEDULER_H
#define SK_SCHEDULER_H

#include <stdint.h>

#include "api/sk_txn.h"
#include "api/sk_entity.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_workflow.h"

// sched flags for sk_sched_create - non RoundRobin routable
#define SK_SCHED_NON_RR_ROUTABLE 0x1

typedef struct sk_sched_t sk_sched_t;

sk_sched_t* sk_sched_create(void* evlp, sk_entity_mgr_t* entity_mgr, int flags);
void sk_sched_destroy(sk_sched_t* sched);

void sk_sched_start(sk_sched_t* sched);
void sk_sched_stop(sk_sched_t* sched);

int sk_sched_setup_bridge(sk_sched_t* src, sk_sched_t* dst);

// deliver a event to dst scheduler
int sk_sched_send(sk_sched_t* sched, sk_sched_t* dst,
                  sk_entity_t* entity, sk_txn_t* txn,
                  uint32_t pto_id, void* proto_msg, int flags);

#endif

