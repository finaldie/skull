#ifndef SK_SCHEDULER_H
#define SK_SCHEDULER_H

#include "api/sk_io.h"
#include "api/sk_io_bridge.h"

#define SK_SCHED_STRATEGY_THROUGHPUT 0
#define SK_SCHED_STRATEGY_LATENCY    1

typedef struct sk_sched_t sk_sched_t;

sk_sched_t* sk_sched_create(void* evlp, int strategy);
void sk_sched_destroy(sk_sched_t* sched);

void sk_sched_start(sk_sched_t* sched);
void sk_sched_stop(sk_sched_t* sched);

int sk_sched_register_io(sk_sched_t* sched, sk_io_t* io);
int sk_sched_register_io_bridge(sk_sched_t* sched,
                                sk_io_bridge_t* io_bridge);

#endif

