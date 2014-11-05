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

#define SKULL_LOG_INFO(log_id, fmt, ...) \
    if (skull_log_enable_info()) { \
        skull_log("[%d] %s; " fmt, \
                  log_id, \
                  skull_log_info_msg(log_id), \
                  __VA_ARGS__); \
    }

#define SKULL_LOG_WARN(log_id, fmt, ...) \
    if (skull_log_enable_warn()) { \
        skull_log("[%d] %s; suggestion:%s; " fmt, \
                  log_id, \
                  skull_log_warn_msg(log_id), \
                  skull_log_warn_solution(log_id), \
                  __VA_ARGS__); \
    }

#define SKULL_LOG_ERROR(log_id, fmt, ...) \
    if (skull_log_enable_error()) { \
        skull_log("[%d] %s; solution:%s; " fmt, \
                  log_id, \
                  skull_log_error_msg(log_id), \
                  skull_log_error_solution(log_id), \
                  __VA_ARGS__); \
    }

#define SKULL_LOG_FATAL(log_id, fmt, ...) \
    if (skull_log_enable_fatal()) { \
        skull_log("[%d] %s; solution:%s; " fmt, \
                  log_id, \
                  skull_log_fatal_msg(log_id), \
                  skull_log_fatal_solution(log_id), \
                  __VA_ARGS__); \
    }

// logging base functions, but user do not need to call them directly
void skull_log(const char* fmt, ...);

bool skull_log_enable_trace();
bool skull_log_enable_debug();
bool skull_log_enable_info();
bool skull_log_enable_warn();
bool skull_log_enable_error();
bool skull_log_enable_fatal();

// get log msg and solution
const char* skull_log_info_msg(int log_id);
const char* skull_log_warn_msg(int log_id);
const char* skull_log_warn_solution(int log_id);
const char* skull_log_error_msg(int log_id);
const char* skull_log_error_solution(int log_id);
const char* skull_log_fatal_msg(int log_id);
const char* skull_log_fatal_solution(int log_id);

#ifdef __cplusplus
}
#endif

#endif

