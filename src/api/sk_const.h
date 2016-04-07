#ifndef SK_CONST_H
#define SK_CONST_H

struct sk_const_t;

// logger cookies
#define SK_CORE_LOG_COOKIE                   "skull.core"

// per-thread log buffer size - 20MB
#define SK_LOG_MAX_PERTHREAD_BUFSIZE         (1024lu * 1024 * 20)

// log file rolling size - 1GB
#define SK_LOG_ROLLING_SIZE                  (1024lu * 1024 * 1024)

// log flush interval - 1 second
#define SK_LOG_FLUSH_INTERVAL                1

// loaders
#define SK_MODULE_NAME_MAX_LEN               1024
#define SK_SERVICE_NAME_MAX_LEN              1024

#define SK_API_LOAD_FUNCNAME                 "skull_load_api"
#define SK_API_UNLOAD_FUNCNAME               "skull_unload_api"

#define SK_USER_LIBNAME_FORMAT               "libskull%s-api.so"
#define SK_USER_PREFIX1                      "/usr/local/lib"
#define SK_USER_PREFIX2                      "/usr/lib"
#define SK_USER_LIBNAME_MAX                  1024

// scheduler
#define SK_SCHED_PULL_NUM                    65535
#define SK_SCHED_MAX_IO_BRIDGE               48
#define SK_SCHED_MAX_ROUTING_HOP             255

// eventloop max events
#define SK_EVENTLOOP_MAX_EVENTS              65535

// thread env name len
#define SK_ENV_NAME_LEN                      24

// config
#define SK_CONFIG_LOCATION_LEN               1024
#define SK_CONFIG_LOGNAME_LEN                1024

#define SK_CONFIG_NO_PORT                    (-1)
#define SK_CONFIG_DEFAULT_CMD_PORT           (7759)
#define SK_CONFIG_VALUE_MAXLEN               256

// ep pool
#define SK_EP_POOL_MAX                       (1024)
#define SK_EP_KEY_MAX                        (64)

#endif

