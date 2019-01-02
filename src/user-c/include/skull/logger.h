#ifndef SKULL_LOGGER_H
#define SKULL_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <skull/logger_private.h>

//  logging helper macros
#define SKULL_LOG_TRACE(...) \
    do { \
        if (skull_log_enable_trace()) { \
            skull_log(SKULL_LOG_PREFIX " TRACE - " __VA_ARGS__); \
        } \
    } while (0)

#define SKULL_LOG_DEBUG(...) \
    do { \
        if (skull_log_enable_debug()) { \
            skull_log(SKULL_LOG_PREFIX " DEBUG - " __VA_ARGS__); \
        } \
    } while (0)

#define SKULL_LOG_INFO(code, fmt, ...) \
    do { \
        if (skull_log_enable_info()) { \
            skull_log(SKULL_LOG_PREFIX " INFO - {%s} " fmt, \
                      code, ##__VA_ARGS__); \
        } \
    } while (0)

#define SKULL_LOG_WARN(code, fmt, ...) \
    do { \
        if (skull_log_enable_warn()) { \
            skull_log(SKULL_LOG_PREFIX " WARN - {%s} " fmt "; suggestion: %s", \
                      code, ##__VA_ARGS__); \
        } \
    } while (0)

#define SKULL_LOG_ERROR(code, fmt, ...) \
    do { \
        if (skull_log_enable_error()) { \
            skull_log(SKULL_LOG_PREFIX " ERROR - {%s} " fmt "; solution: %s", \
                      code, ##__VA_ARGS__); \
        } \
    } while (0)

#define SKULL_LOG_FATAL(code, fmt, ...) \
    do { \
        if (skull_log_enable_fatal()) { \
            skull_log(SKULL_LOG_PREFIX " FATAL - {%s} " fmt "; solution: %s", \
                      code, ##__VA_ARGS__); \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif

