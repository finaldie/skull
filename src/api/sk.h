#ifndef SK_H
#define SK_H

#include <pthread.h>
#include "api/sk_sched.h"
#include "api/sk_config.h"

typedef struct skull_cmd_args_t {
    const char* config_location;
} skull_cmd_args_t;

typedef struct skull_sched_t {
    pthread_t   io_thread;
    sk_sched_t* sched;
} skull_sched_t;

typedef struct skull_core_t {
    skull_cmd_args_t cmd_args;
    sk_config_t*     config;

    skull_sched_t    main_sched;
    skull_sched_t*   worker_sched;

    flist*           workflows;  // element type: sk_workflow_t
} skull_core_t;

void skull_init(skull_core_t* core);
void skull_start(skull_core_t* core);
void skull_stop(skull_core_t* core);

#endif

