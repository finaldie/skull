#ifndef SK_SCHEDULER_H
#define SK_SCHEDULER_H

#include "flist/flist.h"
#include "api/sk_txn.h"
#include "api/sk_entity.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_workflow.h"

typedef struct sk_sched_t sk_sched_t;

sk_sched_t* sk_sched_create(void* evlp, sk_entity_mgr_t* entity_mgr);
void sk_sched_destroy(sk_sched_t* sched);

void sk_sched_start(sk_sched_t* sched);
void sk_sched_stop(sk_sched_t* sched);

int sk_sched_setup_bridge(sk_sched_t* src, sk_sched_t* dst);

// push an event to the scheduler
int sk_sched_push(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn,
                  int pto_id, void* proto_msg);

// deliver a event to another scheduler
int sk_sched_send(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn,
                  int pto_id, void* proto_msg);

#endif

