#include <stdlib.h>

#include "api/sk_const.h"
#include "api/sk_time.h"

typedef struct sk_time_t {
    struct timespec res;

    clockid_t monotonic_id;
    clockid_t monotonic_coarse_id;
} sk_time_t;

static sk_time_t itime = {
    .monotonic_id        = CLOCK_MONOTONIC,
    .monotonic_coarse_id = CLOCK_MONOTONIC
};

void sk_time_init() {
    if (!clock_getres(CLOCK_MONOTONIC_COARSE, &itime.res)) {
        if (itime.res.tv_nsec <= SK_NS_PER_MS) {
            itime.monotonic_coarse_id = CLOCK_MONOTONIC_COARSE;
        }
    }
}

ulong_t sk_time_ns() {
    struct timespec tp;
    if (clock_gettime(itime.monotonic_id, &tp)) {
        return 0;
    }

    return (ulong_t)tp.tv_sec * SK_NS_PER_SEC + (ulong_t)tp.tv_nsec;
}

ulong_t sk_time_res() {
    return (ulong_t)itime.res.tv_nsec;
}

ulong_t sk_time_ms() {
    struct timespec tp;
    if (clock_gettime(itime.monotonic_coarse_id, &tp)) {
        return 0;
    }

    return ((ulong_t)tp.tv_sec * SK_NS_PER_SEC + (ulong_t)tp.tv_nsec)
            / SK_NS_PER_MS;
}
