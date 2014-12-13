#ifndef SKULL_LOGGER_H
#define SKULL_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <skull/skull_logger_private.h>

//  logging helper macros
#define SKULL_LOG_TRACE(fmt, ...) \
    if (skull_log_enable_trace()) { \
        skull_log(SKULL_LOG_PREFIX " TRACE - " fmt, __VA_ARGS__); \
    }

#define SKULL_LOG_DEBUG(fmt, ...) \
    if (skull_log_enable_debug()) { \
        skull_log(SKULL_LOG_PREFIX " DEBUG - " fmt, __VA_ARGS__); \
    }

#define SKULL_LOG_INFO(log_id, fmt, ...) \
    if (skull_log_enable_info()) { \
        skull_log(SKULL_LOG_PREFIX " INFO - [%d] %s; " fmt, \
                  log_id, \
                  skull_log_info_msg(log_id), \
                  __VA_ARGS__); \
    }

#define SKULL_LOG_WARN(log_id, fmt, ...) \
    if (skull_log_enable_warn()) { \
        skull_log(SKULL_LOG_PREFIX " WARN - [%d] %s; suggestion:%s; " fmt, \
                  log_id, \
                  skull_log_warn_msg(log_id), \
                  skull_log_warn_solution(log_id), \
                  __VA_ARGS__); \
    }

#define SKULL_LOG_ERROR(log_id, fmt, ...) \
    if (skull_log_enable_error()) { \
        skull_log(SKULL_LOG_PREFIX " ERROR - [%d] %s; solution:%s; " fmt, \
                  log_id, \
                  skull_log_error_msg(log_id), \
                  skull_log_error_solution(log_id), \
                  __VA_ARGS__); \
    }

#define SKULL_LOG_FATAL(log_id, fmt, ...) \
    if (skull_log_enable_fatal()) { \
        skull_log(SKULL_LOG_PREFIX " FATAL - [%d] %s; solution:%s; " fmt, \
                  log_id, \
                  skull_log_fatal_msg(log_id), \
                  skull_log_fatal_solution(log_id), \
                  __VA_ARGS__); \
    }



#ifdef __cplusplus
}
#endif

#endif

