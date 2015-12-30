#ifndef SK_CONST_H
#define SK_CONST_H

// logger cookies
#define SK_CORE_LOG_COOKIE "skull.core"

// log templates names
#define SK_LOG_INFO_TPL_NAME  "etc/skull-log-info-tpl.yaml"
#define SK_LOG_WARN_TPL_NAME  "etc/skull-log-warn-tpl.yaml"
#define SK_LOG_ERROR_TPL_NAME "etc/skull-log-error-tpl.yaml"
#define SK_LOG_FATAL_TPL_NAME "etc/skull-log-fatal-tpl.yaml"

#define SK_LOG_TPL_NAME_MAX_LEN 1024

// per-thread log buffer size - 20MB
#define SK_LOG_MAX_PERTHREAD_BUFSIZE (1024lu * 1024 * 20)

// log file rolling size - 1GB
#define SK_LOG_ROLLING_SIZE (1024lu * 1024 * 1024)

// log flush interval - 1 second
#define SK_LOG_FLUSH_INTERVAL 1

// loaders
#define SK_MODULE_NAME_MAX_LEN  1024
#define SK_SERVICE_NAME_MAX_LEN 1024

#endif

