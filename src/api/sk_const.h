#ifndef SK_CONST_H
#define SK_CONST_H

struct sk_const_t;

// dump core
#define SK_MAX_BACKTRACE                     (50)

// memory cron job interval (second)
#define SK_MEM_DUMP_INTERVAL                 (5)

// logger cookies
#define SK_CORE_LOG_COOKIE                   "skull.core"

// per-thread log buffer size - 10MB
#define SK_LOG_MAX_PERTHREAD_BUFSIZE         (1024lu * 1024 * 10)

// log file rolling size - 1GB
#define SK_LOG_ROLLING_SIZE                  (1024lu * 1024 * 1024)

// log flush interval - 1 second
#define SK_LOG_FLUSH_INTERVAL                (1)

// log stdout file name
#define SK_LOG_STDOUT_FILE                   "/proc/self/fd/1"

// log full path length
#define SK_LOG_MAX_PATH_LEN                  (1024)

// loaders
#define SK_MODULE_NAME_MAX_LEN               (1024)
#define SK_SERVICE_NAME_MAX_LEN              (1024)

#define SK_API_LOAD_FUNCNAME                 "skull_load_api"
#define SK_API_UNLOAD_FUNCNAME               "skull_unload_api"

#define SK_USER_LIBNAME_FORMAT               "libskull-api-%s.so"
#define SK_USER_PREFIX1                      "/usr/local/lib"
#define SK_USER_PREFIX2                      "/usr/lib"
#define SK_USER_LIBNAME_MAX                  (1024)

// network
//  workflow trigger
#define SK_MAX_LISTEN_BACKLOG                (1024)
#define SK_DEFAULT_READBUF_LEN               (65535)
#define SK_DEFAULT_READBUF_FACTOR            (2)

// scheduler
#define SK_SCHED_PULL_NUM                    (65535)
#define SK_SCHED_MAX_IO_BRIDGE               (48)
#define SK_SCHED_MAX_ROUTING_HOP             (255)
#define SK_SCHED_DEFAULT_WAITMS              (1000)

// eventloop max events
#define SK_EVENTLOOP_MAX_EVENTS              (65535)

// thread env name len
#define SK_ENV_NAME_LEN                      (16)

// config
#define SK_CONFIG_LOCATION_LEN               (1024)
#define SK_CONFIG_LOGNAME_LEN                (128)

#define SK_CONFIG_NO_PORT                    (-1)
#define SK_CONFIG_DEFAULT_CMD_PORT           (7759)
#define SK_CONFIG_VALUE_MAXLEN               (256)

// ep pool
#define SK_EP_POOL_MAX                       (1024)
#define SK_EP_KEY_MAX                        (64)
#define SK_EP_DEFAULT_SHUTDOWN_MS            (1000)

// txn log
#define SK_TXN_DEFAULT_INIT_SIZE             (256)
#define SK_TXN_DEFAULT_MAX_SIZE              (2048)

// service
#define SK_SRV_MAX_TASK                      (1024)

// sk_time
#define SK_NS_PER_SEC                        (1000000000l)
#define SK_NS_PER_MS                         (1000000l)
#define SK_MS_PER_SEC                        (1000l)

// sk_engine
#define SK_ENGINE_SECOND_INTERVAL            (1000)
#define SK_ENGINE_MINUTE_INTERVAL            (1000 * 60)

#endif

