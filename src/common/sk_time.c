#include <stdlib.h>

#include "api/sk_time.h"

ulong_t sk_gettime() {
    struct timespec tp;
    if (clock_gettime(CLOCK_MONOTONIC, &tp)) {
        return 0;
    }

    return (ulong_t)tp.tv_sec * 1000000000l + (ulong_t)tp.tv_nsec;
}
