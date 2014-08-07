#ifndef SK_H
#define SK_H

#include <pthread.h>
#include "api/sk_sched.h"

#define SKULL_WORKER_NUM 2

typedef struct skull_sched_t {
    pthread_t   io_thread;
    sk_sched_t* sched;
} skull_sched_t;

typedef struct skull_core_t {
    skull_sched_t main_sched;
    skull_sched_t worker_sched[SKULL_WORKER_NUM];
} skull_core_t;

void skull_init(skull_core_t* core);
void skull_start(skull_core_t* core);
void skull_stop(skull_core_t* core);

#endif

