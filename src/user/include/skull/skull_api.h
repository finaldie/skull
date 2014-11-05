#ifndef SKULL_API_H
#define SKULL_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

// ================== Logging Part ========================
// publis:
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

// private:
//  internal macros
#define SKULL_TO_STR(x) #x
#define SKULL_EXTRACT_STR(x) SKULL_TO_STR(x)
#define SKULL_LOG_PREFIX __FILE__ ":" SKULL_EXTRACT_STR(__LINE__)

// private:
//  logging base functions, but user do not need to call them directly
void skull_log(const char* fmt, ...);

bool skull_log_enable_trace();
bool skull_log_enable_debug();
bool skull_log_enable_info();
bool skull_log_enable_warn();
bool skull_log_enable_error();
bool skull_log_enable_fatal();

// private:
//  get log msg and solution
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

