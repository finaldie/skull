#ifndef SK_CORE_H
#define SK_CORE_H

#include <stdbool.h>

#include "flibs/flist.h"
#include "flibs/fhash.h"

#include "api/sk_config.h"
#include "api/sk_log.h"
#include "api/sk_log_tpl.h"
#include "api/sk_mon.h"
#include "api/sk_service.h"
#include "api/sk_engine.h"
#include "api/sk_trigger.h"

typedef enum sk_core_status_t {
    SK_CORE_INIT       = 0,
    SK_CORE_STARTING   = 1,
    SK_CORE_RUNNING    = 2,
    SK_CORE_STOPPING   = 3,
    SK_CORE_DESTROYING = 4
} sk_core_status_t;

// skull core related structures
typedef struct sk_cmd_args_t {
    const char* binary_path;
    const char* config_location;
    bool daemon;

#if __WORDSIZE == 64
    int _padding :24;
    int _padding1;
#else
    int _padding :24;
#endif
} sk_cmd_args_t;

// Core data structure
// NOTES: All the members must be thread-safe for read action after
//  initialization
typedef struct sk_core_t {
    // ======= private =======
    sk_mon_t*        mon;
    sk_mon_t*        umon; // user mon

    // ======= public  =======
    sk_cmd_args_t    cmd_args;
    sk_config_t*     config;

    sk_engine_t*     master;
    sk_engine_t**    workers;

    // user bio(s)
    sk_engine_t**    bio;

    // logger
    sk_logger_t*     logger;

    flist*           workflows;      // element type: sk_workflow_t*
    flist*           triggers;       // emement type: sk_trigger_t*
    fhash*           unique_modules; // key: module name; value: sk_module_t*
    fhash*           services;       // key: service name; value: sk_service_t*
    const char*      working_dir;

    // admin module and config
    sk_workflow_cfg_t* admin_wf_cfg;
    sk_workflow_t*     admin_wf;

    // skull core status
    sk_core_status_t status;

    // max open files limitation
    int max_fds;
} sk_core_t;

void sk_core_init(sk_core_t* core);
void sk_core_start(sk_core_t* core);
void sk_core_stop(sk_core_t* core);
void sk_core_destroy(sk_core_t* core);

// utils
sk_service_t*    sk_core_service(sk_core_t*, const char* service_name);
sk_core_status_t sk_core_status(sk_core_t*);
sk_engine_t*     sk_core_bio(sk_core_t*, int idx);
const char*      sk_core_binpath(sk_core_t*);

#endif

