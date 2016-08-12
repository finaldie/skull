#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <execinfo.h>
#include <unistd.h>

#include "api/sk_const.h"
#include "api/sk_utils.h"

void sk_assert_exit(const char* expr, const char* file, int lineno)
{
    printf("FATAL: assert [%s] failed - %s:%d\n", expr, file, lineno);

#ifdef SK_DUMP_CORE
    sk_backtrace_print();
    exit(1);
#else
    abort();
#endif
}

void sk_assert_exit_with_msg(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

#ifdef SK_DUMP_CORE
    sk_backtrace_print();
    exit(1);
#else
    abort();
#endif
}

void sk_backtrace_print()
{
    void*  buffer[SK_MAX_BACKTRACE];
    int    size;

    size = backtrace(buffer, SK_MAX_BACKTRACE);
    backtrace_symbols_fd(buffer, size, STDERR_FILENO);
}
