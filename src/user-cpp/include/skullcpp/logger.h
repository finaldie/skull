#ifndef SKULLCPP_LOGGER_H
#define SKULLCPP_LOGGER_H

#include <skull/logger.h>
#include <sstream>

//  logging helper macros
#define SKULLCPP_LOG_TRACE(message) \
    do { \
        if (skull_log_enable_trace()) { \
            std::ostringstream oss; \
            oss << message; \
            skull_log(SKULL_LOG_PREFIX " TRACE - %s", oss.str().c_str()); \
        } \
    } while (0)

#define SKULLCPP_LOG_DEBUG(message) \
    do { \
        if (skull_log_enable_debug()) { \
            std::ostringstream oss; \
            oss << message; \
            skull_log(SKULL_LOG_PREFIX " DEBUG - %s", oss.str().c_str()); \
        } \
    } while (0)

#define SKULLCPP_LOG_INFO(code, message) \
    do { \
        if (skull_log_enable_info()) { \
            std::ostringstream oss; \
            oss << message; \
            skull_log(SKULL_LOG_PREFIX " INFO - {%s} %s", \
                      code, oss.str().c_str()); \
        } \
    } while (0)

#define SKULLCPP_LOG_WARN(code, message, suggestion) \
    do { \
        if (skull_log_enable_warn()) { \
            std::ostringstream oss_msg; \
            std::ostringstream oss_sug; \
            oss_msg << message; \
            oss_sug << suggestion; \
            skull_log(SKULL_LOG_PREFIX " WARN - {%s} %s; suggestion: %s", \
                      code, oss_msg.str().c_str(), oss_sug.str().c_str()); \
        } \
    } while (0)

#define SKULLCPP_LOG_ERROR(code, message, solution) \
    do { \
        if (skull_log_enable_error()) { \
            std::ostringstream oss_msg; \
            std::ostringstream oss_sol; \
            oss_msg << message; \
            oss_sol << solution; \
            skull_log(SKULL_LOG_PREFIX " ERROR - {%s} %s; solution: %s", \
                      code, oss_msg.str().c_str(), oss_sol.str().c_str()); \
        } \
    } while (0)

#define SKULLCPP_LOG_FATAL(code, message, solution) \
    do { \
        if (skull_log_enable_fatal()) { \
            std::ostringstream oss_msg; \
            std::ostringstream oss_sol; \
            oss_msg << message; \
            oss_sol << solution; \
            skull_log(SKULL_LOG_PREFIX " FATAL - {%s} %s; solution: %s", \
                      code, oss_msg.str().c_str(), oss_sol.str().c_str()); \
        } \
    } while (0)

#endif
