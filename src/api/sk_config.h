#ifndef SK_CONFIG_H
#define SK_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

#include "flibs/flist.h"
#include "flibs/fhash.h"
#include "api/sk_const.h"
#include "api/sk_types.h"
#include "api/sk_service_data.h"

#define SK_SOCK_TCP 0
#define SK_SOCK_UDP 1
#define SK_SOCK_MAX 2 // Keep this at the end of all types

typedef struct sk_module_cfg_t {
    const char* name;
    const char* type;
} sk_module_cfg_t;

typedef struct sk_workflow_cfg_t {
    uint32_t concurrent   :1;
    uint32_t enable_stdin :1;
    uint32_t sock_type    :5;   // Transport layer type: TCP (by default), UDP
    uint32_t _reserved    :25;

    int      port;

    // The IPv4 address we want to bind (by default is 127.0.0.1)
    // TODO: Support IPv6 address
    const char* bind;

    const char* idl_name; // workflow idl name
    flist* modules;       // sk_module_cfg_t list

    // unit: millisecond, if the execute time > this threshold, engine will
    //  cancel this transaction. Default value is 0 (unlimited)
    int      timeout;

#if __WORDSIZE == 64
    int      __padding;
#endif
} sk_workflow_cfg_t;

typedef struct sk_service_cfg_t {
    // type of service (cpp, ...)
    const char* type;
    bool enable;

    // padding 3 bytes
    char  __padding1;
    short __padding2;

    sk_srv_data_mode_t data_mode;

    // Max queue size
    int max_qsize;

#if __WORDSIZE == 64
    int   __padding3;
#endif
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

    // number of bio(s)
    int    bio_cnt;

    int    command_port;

    // Supportted languages, value: char*
    flist* langs;

    // max open fds
    int    max_fds;

#if __WORDSIZE == 64
    int    __padding;
#endif
} sk_config_t;

sk_config_t* sk_config_create(const char* filename);
void sk_config_destroy(sk_config_t* config);

void sk_config_print(sk_config_t* config);

#endif

