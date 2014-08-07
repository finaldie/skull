#ifndef SK_SCHEDULER_H
#define SK_SCHEDULER_H

#include "api/sk_entity.h"

typedef struct sk_sched_t sk_sched_t;

sk_sched_t* sk_sched_create();
void sk_sched_destroy(sk_sched_t* sched);

void sk_sched_start(sk_sched_t* sched);
void sk_sched_stop(sk_sched_t* sched);
int sk_sched_setup_bridge(sk_sched_t* src, sk_sched_t* dst);

void* sk_sched_eventloop(sk_sched_t* sched);

// push an event to the scheduler
int sk_sched_push(sk_sched_t* sched, int fd, int pto_id, void* proto_msg);

// deliver a event to another scheduler
int sk_sched_send(sk_sched_t* sched, int fd, int pto_id, void* proto_msg);

#endif

