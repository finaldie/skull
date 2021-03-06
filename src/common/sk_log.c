#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "flibs/flog.h"
#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_env.h"
#include "api/sk_log.h"
#include "api/sk_metrics.h"

// INTERNAL APIs
static
void _skull_log_notification_cb(flog_event_t event)
{
    switch (event) {
    case FLOG_EVENT_ERROR_WRITE:
        sk_metrics_global.log_error_write.inc(1);
        sk_print_err("Fatal: skull write log occur errors!\n");
        break;
    case FLOG_EVENT_ERROR_ASYNC_PUSH:
        sk_metrics_global.log_error_async_push.inc(1);
        sk_print_err("Fatal: skull async push occur errors!\n");
        break;
    case FLOG_EVENT_ERROR_ASYNC_POP:
        sk_metrics_global.log_error_async_pop.inc(1);
        sk_print_err("Fatal: skull async pop occur errors!\n");
        break;
    case FLOG_EVENT_TRUNCATED:
        sk_metrics_global.log_truncated.inc(1);
        sk_print_err("Fatal: skull write log which was truncated!\n");
        break;
    case FLOG_EVENT_BUFFER_FULL:
        sk_metrics_global.log_buffer_full.inc(1);
        sk_print_err("Fatal: skull logger buffer is full!\n");
        break;
    case FLOG_EVENT_USER_BUFFER_RELEASED:
        sk_print("Info: skull logger quit gracefully\n");
        break;
    default:
        sk_print_err("Fatal: unknow skull log event %d\n", event);
        break;
    }
}

// Internal
static
sk_logger_t* _logger_create(const char* log_path,
                            bool  stdout_fwd) {
    flog_file_t* logger = flog_create(log_path, FLOG_F_ASYNC);
    SK_ASSERT_MSG(logger, "Logger creation failure: %s : %s\n",
                  log_path, strerror(errno));

    return logger;
}

static
void _logger_apply(flog_file_t* logger, int log_level, bool rolling_disabled, bool stdout_fwd) {
    if (stdout_fwd) {
        rolling_disabled = true; // Force set it to true
    }

    // set log level
    flog_set_level(logger, log_level);

    // set flush inverval: 1 second
    flog_set_flush_interval(logger, SK_LOG_FLUSH_INTERVAL);

    // set file rolling size: 1GB
    flog_set_rolling_size(logger, rolling_disabled ? 0 : SK_LOG_ROLLING_SIZE);

    // set log buffer size(per-thread): 10MB
    flog_set_buffer_size(SK_LOG_MAX_PERTHREAD_BUFSIZE);

    // set up the notification callback, we can handle it if there are some
    //  abnormal things happened
    flog_register_event(_skull_log_notification_cb);
}

// Public APIs
sk_logger_t* sk_logger_create(const char* log_path,
                              int  log_level,
                              bool rolling_disabled,
                              bool stdout_fwd)
{
    flog_file_t* logger = _logger_create(log_path, stdout_fwd);

    _logger_apply(logger, log_level, rolling_disabled, stdout_fwd);

    return logger;
}

void sk_logger_destroy(sk_logger_t* logger)
{
    if (!logger) {
        return;
    }

    flog_destroy(logger);
}

void sk_logger_setcookie(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    flog_vset_cookie(fmt, ap);
    va_end(ap);
}
