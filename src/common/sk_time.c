#include <stdlib.h>

#include "flibs/compiler.h"
#include "api/sk_const.h"
#include "api/sk_time.h"

typedef struct sk_time_t {
    struct timespec res;
    struct timespec res_coarse;
    struct timespec res_thread_cpu;

    clockid_t monotonic_id;
    clockid_t monotonic_coarse_id;
    clockid_t thread_cpu_id;

    int       _padding;
} sk_time_t;

static sk_time_t itime = {
    .monotonic_id        = CLOCK_MONOTONIC,
    .monotonic_coarse_id = CLOCK_MONOTONIC,
    .thread_cpu_id       = CLOCK_THREAD_CPUTIME_ID
};

void sk_time_init() {
    clock_getres(CLOCK_MONOTONIC, &itime.res);

    if (!clock_getres(CLOCK_MONOTONIC_COARSE, &itime.res_coarse)) {
        if (itime.res_coarse.tv_nsec <= SK_NS_PER_MS) {
            itime.monotonic_coarse_id = CLOCK_MONOTONIC_COARSE;
        }
    }

    clock_getres(CLOCK_THREAD_CPUTIME_ID, &itime.res_thread_cpu);
}

sk_time_info_t* sk_time_info(sk_time_info_t* info) {
    if (!info) return NULL;

    info->res                 = itime.res;
    info->res_coarse          = itime.res_coarse;
    info->monotonic_id        = itime.monotonic_id;
    info->monotonic_coarse_id = itime.monotonic_coarse_id;
    return info;
}

slong_t sk_time_ns() {
    struct timespec tp;
    if (unlikely(clock_gettime(itime.monotonic_id, &tp))) {
        return 0;
    }

    return (slong_t)tp.tv_sec * SK_NS_PER_SEC + (slong_t)tp.tv_nsec;
}

slong_t sk_time_us() {
    return sk_time_ns() / SK_NS_PER_US;
}

slong_t sk_time_ms() {
    struct timespec tp;
    if (unlikely(clock_gettime(itime.monotonic_coarse_id, &tp))) {
        return 0;
    }

    return ((slong_t)tp.tv_sec * SK_NS_PER_SEC + (slong_t)tp.tv_nsec)
            / SK_NS_PER_MS;
}

slong_t sk_time_threadcpu_ns() {
    struct timespec tp;
    if (unlikely(clock_gettime(itime.thread_cpu_id, &tp))) {
        return 0;
    }

    return (slong_t)tp.tv_sec * SK_NS_PER_SEC + (slong_t)tp.tv_nsec;
}

