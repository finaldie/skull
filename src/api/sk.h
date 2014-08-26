#ifndef SK_H
#define SK_H

#include <pthread.h>

#include "flist/flist.h"
#include "fhash/fhash.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_sched.h"
#include "api/sk_config.h"

typedef struct skull_cmd_args_t {
    const char* config_location;
} skull_cmd_args_t;

typedef struct skull_sched_t {
    pthread_t        io_thread;
    void*            evlp;
    sk_sched_t*      sched;
    sk_entity_mgr_t* entity_mgr;
} skull_sched_t;

typedef struct skull_core_t {
    skull_cmd_args_t cmd_args;
    sk_config_t*     config;

    skull_sched_t    main_sched;
    skull_sched_t*   worker_sched;

    // shared data
    flist*           workflows;  // element type: sk_workflow_t
    fhash*           unique_modules;
    const char*      working_dir;
} skull_core_t;

void skull_init(skull_core_t* core);
void skull_start(skull_core_t* core);
void skull_stop(skull_core_t* core);

#endif

