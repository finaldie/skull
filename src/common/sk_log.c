#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flibs/flog.h"
#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_env.h"
#include "api/sk_log.h"

// INTERNAL APIs
static
void _skull_log_notification_cb(flog_event_t event)
{
    switch (event) {
    case FLOG_EVENT_ERROR_WRITE:
        sk_print_err("Fatal: skull write log occur errors!\n");
        break;
    case FLOG_EVENT_ERROR_ASYNC_PUSH:
        sk_print_err("Fatal: skull async push occur errors!\n");
        break;
    case FLOG_EVENT_ERROR_ASYNC_POP:
        sk_print_err("Fatal: skull async pop occur errors!\n");
        break;
    case FLOG_EVENT_TRUNCATED:
        sk_print_err("Fatal: skull write log which was truncated!\n");
        break;
    case FLOG_EVENT_BUFFER_FULL:
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

// Public APIs
sk_logger_t* sk_logger_create(const char* workdir,
                              const char* log_name,
                              int log_level)
{
    size_t workdir_len = strlen(workdir);
    size_t log_name_len = strlen(log_name);
    // notes: we add more 5 bytes space for the string of fullname "/log/"
    size_t full_name_sz = workdir_len + log_name_len + 5 + 1;

    // 1. construct the full log name and then create async logger
    // NOTES: the log file will be put at log/xxx
    char full_log_name[full_name_sz];
    snprintf(full_log_name, full_name_sz, "%s/log/%s",
             workdir, log_name);

    flog_file_t* logger = flog_create(full_log_name, FLOG_F_ASYNC);
    SK_ASSERT_MSG(logger, "logger create failure");

    // 2. set log level
    flog_set_level(log_level);

    // 3. set flush inverval: 1 second
    flog_set_flush_interval(SK_LOG_FLUSH_INTERVAL);

    // 4. set file rolling size: 1GB
    flog_set_roll_size(SK_LOG_ROLLING_SIZE);

    // 5. set log buffer size(per-thread): 20MB
    flog_set_buffer_size(SK_LOG_MAX_PERTHREAD_BUFSIZE);

    // 6. set up the notification callback, we can handle it if there are some
    // abnormal things happened
    flog_register_event_callback(_skull_log_notification_cb);

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
