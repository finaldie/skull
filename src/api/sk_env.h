#ifndef SK_ENV_H
#define SK_ENV_H

#include <pthread.h>

#include "flist/flist.h"
#include "fhash/fhash.h"

#include "api/sk_entity.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_sched.h"
#include "api/sk_config.h"
#include "api/sk_log.h"
#include "api/sk_log_tpl.h"
#include "api/sk_mon.h"

// per-thread data and macros, most of time, normally you only need to use these macros
#define SK_ENV            (sk_thread_env())
#define SK_ENV_CORE       (sk_thread_env()->core)

#define SK_ENV_SCHED      (sk_thread_env()->sched->sched)
#define SK_ENV_ENTITY_MGR (sk_thread_env()->sched->entity_mgr)
#define SK_ENV_EVENTLOOP  (sk_thread_env()->sched->evlp)
#define SK_ENV_WORKFLOWS  (sk_thread_env()->core->workflows)
#define SK_ENV_LOGGER     (sk_thread_env()->core->logger)
#define SK_ENV_MON        (sk_thread_env()->sched->mon)

// skull core related structures
typedef struct skull_cmd_args_t {
    const char* config_location;
} skull_cmd_args_t;

typedef struct skull_sched_t {
    pthread_t        io_thread;
    void*            evlp;
    sk_sched_t*      sched;
    sk_entity_mgr_t* entity_mgr;
    sk_mon_t*        mon;
} skull_sched_t;

typedef struct skull_core_t {
    // ======= private =======
    sk_mon_t*        mon;
    sk_mon_t*        umon; // user mon

    // ======= public  =======
    skull_cmd_args_t cmd_args;
    sk_config_t*     config;

    skull_sched_t    main_sched;
    skull_sched_t*   worker_sched;

    // logger
    sk_logger_t*     logger;

    // log templates
    sk_log_tpl_t*    info_log_tpl;
    sk_log_tpl_t*    warn_log_tpl;
    sk_log_tpl_t*    error_log_tpl;
    sk_log_tpl_t*    fatal_log_tpl;

    flist*           workflows;      // element type: sk_workflow_t
    fhash*           unique_modules; // key: module name; value: sk_module_t
    const char*      working_dir;
} skull_core_t;

#define SK_ENV_NAME_LEN 20

typedef struct sk_thread_env_t {
    // ======== public  ========
    skull_core_t*    core;
    skull_sched_t*   sched;

    // used for logging or debugging
    char name[SK_ENV_NAME_LEN];
    int  idx;
} sk_thread_env_t;

void sk_thread_env_init();
void sk_thread_env_set(sk_thread_env_t* env);
sk_thread_env_t* sk_thread_env();

#endif

