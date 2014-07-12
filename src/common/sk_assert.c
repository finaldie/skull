#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void sk_assert_exit(const char* expr, const char* file, int lineno)
{
    printf("FATAL ERROR: [%s] %s:%d, error: %s\n",
           expr, file, lineno, strerror(errno));
    exit(errno);
}
