#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "api/sk_utils.h"

void sk_assert_exit(const char* expr, const char* file, int lineno)
{
    printf("FATAL: assert [%s] failed - %s:%d, error: %s\n",
           expr, file, lineno, strerror(errno));
    abort();
}

void sk_assert_exit_with_msg(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    abort();
}
