#ifndef SK_CORE_H
#define SK_CORE_H

#include "flibs/flist.h"
#include "flibs/fhash.h"

#include "api/sk_config.h"
#include "api/sk_log.h"
#include "api/sk_log_tpl.h"
#include "api/sk_mon.h"
#include "api/sk_engine.h"
#include "api/sk_trigger.h"

// skull core related structures
typedef struct sk_cmd_args_t {
    const char* config_location;
} sk_cmd_args_t;

typedef struct sk_core_t {
    // ======= private =======
    sk_mon_t*        mon;
    sk_mon_t*        umon; // user mon

    // ======= public  =======
    sk_cmd_args_t    cmd_args;
    sk_config_t*     config;

    sk_engine_t*     master;
    sk_engine_t**    workers;

    // logger
    sk_logger_t*     logger;

    // log templates
    sk_log_tpl_t*    info_log_tpl;
    sk_log_tpl_t*    warn_log_tpl;
    sk_log_tpl_t*    error_log_tpl;
    sk_log_tpl_t*    fatal_log_tpl;

    flist*           workflows;      // element type: sk_workflow_t
    flist*           triggers;       // emement type: sk_trigger_t
    fhash*           unique_modules; // key: module name; value: sk_module_t
    const char*      working_dir;
} sk_core_t;

void sk_core_init(sk_core_t* core);
void sk_core_start(sk_core_t* core);
void sk_core_stop(sk_core_t* core);
void sk_core_destroy(sk_core_t* core);

#endif

