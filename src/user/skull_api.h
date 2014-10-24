#ifndef SKULL_API_H
#define SKULL_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>

// ================== Logging Part ========================
// logging helper macros
#define SKULL_LOG_TRACE(fmt, ...) \
    if (skull_log_enable_trace()) { \
        skull_log(fmt, __VA_ARGS__); \
    }

#define SKULL_LOG_DEBUG(fmt, ...) \
    if (skull_log_enable_debug()) { \
        skull_log(fmt, __VA_ARGS__); \
    }

#define SKULL_LOG_INFO(fmt, ...) \
    if (skull_log_enable_info()) { \
        skull_log(fmt, __VA_ARGS__); \
    }

#define SKULL_LOG_WARN(fmt, ...) \
    if (skull_log_enable_warn()) { \
        skull_log(fmt, __VA_ARGS__); \
    }

#define SKULL_LOG_ERROR(fmt, ...) \
    if (skull_log_enable_error()) { \
        skull_log(fmt, __VA_ARGS__); \
    }

#define SKULL_LOG_FATAL(fmt, ...) \
    if (skull_log_enable_fatal()) { \
        skull_log(fmt, __VA_ARGS__); \
    }

// logging base functions, but user do not need to call it directly
void skull_log(const char* fmt, ...);
bool skull_log_enable_trace();
bool skull_log_enable_debug();
bool skull_log_enable_info();
bool skull_log_enable_warn();
bool skull_log_enable_error();
bool skull_log_enable_fatal();


#ifdef __cplusplus
}
#endif

#endif

