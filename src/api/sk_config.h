#ifndef SK_CONFIG_H
#define SK_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

#include "flibs/flist.h"
#include "flibs/fhash.h"
#include "api/sk_types.h"
#include "api/sk_module.h"

#define SK_CONFIG_LOCATION_LEN    1024
#define SK_CONFIG_LOGNAME_LEN	  1024

#define SK_CONFIG_NO_PORT         -1

typedef struct sk_workflow_cfg_t {
    int concurrent;
    int port;

    const char* idl_name; // workflow idl name
    flist* modules;       // module name list
} sk_workflow_cfg_t;

typedef struct sk_service_cfg_t {
    bool enable;
} sk_service_cfg_t;

typedef struct sk_config_t {
    // the location of the config file, while this is root location of the
    // runtime environment as well
    char location[SK_CONFIG_LOCATION_LEN];

    // log name (we don't specify the log location, by default the log file will
    // be put at the `log` folder)
    char log_name[SK_CONFIG_LOGNAME_LEN];

    // how many worker threads will be created after skull starting
    int threads;

    // log level of flog, from LOG_LEVEL_TRACE(0) - LOG_LEVEL_FATAL(5)
    int log_level;

    // sk_workflow_cfg_t list
    flist* workflows;

    // key: service name, value: sk_service_t*
    fhash* services;
} sk_config_t;

sk_config_t* sk_config_create(const char* filename);
void sk_config_destroy(sk_config_t* config);

void sk_config_print(sk_config_t* config);

#endif

