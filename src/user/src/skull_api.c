#include <stdlib.h>

#include "flog/flog.h"
#include "api/sk_log.h"
#include "api/sk_env.h"

#include "skull/skull_api.h"

void skull_log(const char* fmt, ...)
{
    sk_logger_t* logger = SK_THREAD_ENV_LOGGER;

    va_list ap;
    va_start(ap, fmt);

    flog_vwritef(logger, fmt, ap);
    va_end(ap);
}

bool skull_log_enable_trace()
{
    return flog_is_trace_enabled();
}

bool skull_log_enable_debug()
{
    return flog_is_debug_enabled();
}

bool skull_log_enable_info()
{
    return flog_is_info_enabled();
}

bool skull_log_enable_warn()
{
    return flog_is_warn_enabled();
}

bool skull_log_enable_error()
{
    return flog_is_error_enabled();
}

bool skull_log_enable_fatal()
{
    return flog_is_fatal_enabled();
}
