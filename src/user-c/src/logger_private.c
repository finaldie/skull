#include <stdlib.h>

#include "flibs/flog.h"
#include "api/sk_log.h"
#include "api/sk_env.h"

#include "skull/logger_private.h"

void skull_log(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    flog_vwritef(SK_ENV_LOGGER, fmt, ap);
    va_end(ap);
}

bool skull_log_enable_trace()
{
    return flog_is_trace_enabled(SK_ENV_LOGGER);
}

bool skull_log_enable_debug()
{
    return flog_is_debug_enabled(SK_ENV_LOGGER);
}

bool skull_log_enable_info()
{
    return flog_is_info_enabled(SK_ENV_LOGGER);
}

bool skull_log_enable_warn()
{
    return flog_is_warn_enabled(SK_ENV_LOGGER);
}

bool skull_log_enable_error()
{
    return flog_is_error_enabled(SK_ENV_LOGGER);
}

bool skull_log_enable_fatal()
{
    return flog_is_fatal_enabled(SK_ENV_LOGGER);
}

