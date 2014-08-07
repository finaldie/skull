#ifndef SK_UTILS_H
#define SK_UTILS_H

#include <stdio.h>
#include <stdarg.h>

// INTERNALs
#define TO_STR(x) #x


// APIs
#define SK_ASSERT(expr) \
    if (!(expr)) { \
        sk_assert_exit(#expr, __FILE__, __LINE__); \
    }

#define SK_ASSERT_MSG(expr, format, ...) \
    if (!(expr)) { \
        va_list args; \
        va_start(args, format); \
        sk_assert_exit_with_msg("FATAL: assert [" #expr "] failed - " \
                                __FILE__ ":" TO_STR(__LINE__) format, \
                                #expr, __FILE__, __LINE__ , args); \
        va_end(args); \
    }

void sk_assert_exit(const char* expr, const char* file, int lineno);
void sk_assert_exit_with_msg(const char* format, va_list args);

#endif

