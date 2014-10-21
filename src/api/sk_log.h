#ifndef SK_LOG_H
#define SK_LOG_H

#include <stdarg.h>
#include "flog/flog.h"

typedef flog_file_t sk_logger_t;

#define SK_LOG_TRACE(logger, ...) FLOG_TRACE(logger, __VA_ARGS__);
#define SK_LOG_DEBUG(logger, ...) FLOG_DEBUG(logger, __VA_ARGS__);
#define SK_LOG_INFO(logger, ...)  FLOG_INFO(logger, __VA_ARGS__);
#define SK_LOG_WARN(logger, ...)  FLOG_WARN(logger, __VA_ARGS__);
#define SK_LOG_ERROR(logger, ...) FLOG_ERROR(logger, __VA_ARGS__);
#define SK_LOG_FATAL(logger, ...) FLOG_FATAL(logger, __VA_ARGS__);

sk_logger_t* sk_create_logger(const char* workdir,
                              const char* log_name,
                              int log_level);

void sk_destroy_logger(sk_logger_t* logger);

#endif

