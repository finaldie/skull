#include <stdlib.h>

#include "api/sk_const.h"
#include "api/sk_time.h"

typedef struct sk_time_t {
    struct timespec res;
    struct timespec res_coarse;

    clockid_t monotonic_id;
    clockid_t monotonic_coarse_id;
} sk_time_t;

static sk_time_t itime = {
    .monotonic_id        = CLOCK_MONOTONIC,
    .monotonic_coarse_id = CLOCK_MONOTONIC
};

void sk_time_init() {
    clock_getres(CLOCK_MONOTONIC, &itime.res);

    if (!clock_getres(CLOCK_MONOTONIC_COARSE, &itime.res_coarse)) {
        if (itime.res_coarse.tv_nsec <= SK_NS_PER_MS) {
            itime.monotonic_coarse_id = CLOCK_MONOTONIC_COARSE;
        }
    }
}

sk_time_info_t* sk_time_info(sk_time_info_t* info) {
    if (!info) return NULL;

    info->res                 = itime.res;
    info->res_coarse          = itime.res_coarse;
    info->monotonic_id        = itime.monotonic_id;
    info->monotonic_coarse_id = itime.monotonic_coarse_id;
    return info;
}

ulong_t sk_time_ns() {
    struct timespec tp;
    if (clock_gettime(itime.monotonic_id, &tp)) {
        return 0;
    }

    return (ulong_t)tp.tv_sec * SK_NS_PER_SEC + (ulong_t)tp.tv_nsec;
}

ulong_t sk_time_ms() {
    struct timespec tp;
    if (clock_gettime(itime.monotonic_coarse_id, &tp)) {
        return 0;
    }

    return ((ulong_t)tp.tv_sec * SK_NS_PER_SEC + (ulong_t)tp.tv_nsec)
            / SK_NS_PER_MS;
}

