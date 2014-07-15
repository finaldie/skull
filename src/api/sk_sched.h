#ifndef SK_SCHEDULER_H
#define SK_SCHEDULER_H

#include "api/sk_event.h"
#include "api/sk_io.h"
#include "api/sk_io_bridge.h"

#define SK_IO_NET_ACCEPT 0
#define SK_IO_NET_SOCK   1

#define SK_IO_STAT_READY 0
#define SK_IO_STAT_PAUSE 1

typedef struct sk_sched_t sk_sched_t;

typedef struct sk_sched_io_t {
    int type;
    int status;
    sk_io_t* io;
} sk_sched_io_t;

typedef struct sk_sched_opt_t {
    sk_event_opt_t**  pto_tbl;
} sk_sched_opt_t;

sk_sched_t* sk_sched_create(void* evlp, sk_sched_opt_t opt);
void sk_sched_destroy(sk_sched_t* sched);

void sk_sched_start(sk_sched_t* sched);
void sk_sched_stop(sk_sched_t* sched);

int sk_sched_reg_io(sk_sched_t* sched, int io_type, sk_io_t* io);
int sk_sched_reg_io_bridge(sk_sched_t* sched, sk_io_bridge_t* io_bridge);

void* sk_sched_get_eventloop(sk_sched_t* sched);
int sk_sched_push(sk_sched_t* sched, sk_event_t* event);

// main and worker scheduler

#define SK_SCHED_STRATEGY_THROUGHPUT 0
#define SK_SCHED_STRATEGY_LATENCY    1

sk_sched_t* sk_main_sched_create(void* evlp, int strategy);
sk_sched_t* sk_worker_sched_create(void* evlp, int strategy);

#endif

