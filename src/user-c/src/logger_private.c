#include <stdlib.h>

#include "flog/flog.h"
#include "api/sk_log.h"
#include "api/sk_log_tpl.h"
#include "api/sk_env.h"

#include "skull/skull_api.h"

void skull_log(const char* fmt, ...)
{
    sk_logger_t* logger = SK_ENV_LOGGER;

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

const char* skull_log_info_msg(int log_id)
{
    sk_log_tpl_t* info_tpl = SK_ENV_CORE->info_log_tpl;
    return sk_log_tpl_msg(info_tpl, log_id);
}

const char* skull_log_warn_msg(int log_id)
{
    sk_log_tpl_t* warn_tpl = SK_ENV_CORE->warn_log_tpl;
    return sk_log_tpl_msg(warn_tpl, log_id);
}

const char* skull_log_warn_solution(int log_id)
{
    sk_log_tpl_t* warn_tpl = SK_ENV_CORE->warn_log_tpl;
    return sk_log_tpl_solution(warn_tpl, log_id);
}

const char* skull_log_error_msg(int log_id)
{
    sk_log_tpl_t* error_tpl = SK_ENV_CORE->error_log_tpl;
    return sk_log_tpl_msg(error_tpl, log_id);
}

const char* skull_log_error_solution(int log_id)
{
    sk_log_tpl_t* error_tpl = SK_ENV_CORE->error_log_tpl;
    return sk_log_tpl_solution(error_tpl, log_id);
}

const char* skull_log_fatal_msg(int log_id)
{
    sk_log_tpl_t* fatal_tpl = SK_ENV_CORE->fatal_log_tpl;
    return sk_log_tpl_msg(fatal_tpl, log_id);
}

const char* skull_log_fatal_solution(int log_id)
{
    sk_log_tpl_t* fatal_tpl = SK_ENV_CORE->fatal_log_tpl;
    return sk_log_tpl_solution(fatal_tpl, log_id);
}
