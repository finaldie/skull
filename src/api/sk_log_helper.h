#ifndef SK_LOG_HELPER
#define SK_LOG_HELPER

#include "api/sk_env.h"
#include "api/sk_log.h"

#define SK_LOG_SETCOOKIE(fmt, ...) \
    sk_logger_setcookie("%s:" fmt, SK_ENV->name, __VA_ARGS__)

#endif

