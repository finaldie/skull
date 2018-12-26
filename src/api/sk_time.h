#ifndef SK_TIME_H
#define SK_TIME_H

#include <time.h>

#include "api/sk_types.h"

typedef struct sk_time_info_t {
    struct timespec res;
    struct timespec res_coarse;

    clockid_t monotonic_id;
    clockid_t monotonic_coarse_id;
} sk_time_info_t;

void sk_time_init();

sk_time_info_t* sk_time_info(sk_time_info_t*);

/**
 * Unit: nanoseconds. clockid = CLOCK_MONOTONIC by default
 *
 * @return  0: error.
 *         >0: succeed, return the nanoseconds
 */
slong_t sk_time_ns();

/**
 * Unit: microseconds. clockid = CLOCK_MONOTONIC by default
 *
 * @return  0: error.
 *         >0: succeed, return the microseconds
 */
slong_t sk_time_us();

/**
 * Unit: milliseconds. clockid = CLOCK_MONOTONIC_COARSE by default
 *
 * @note clockid could be downgraded to CLOCK_MONOTONIC if resolution is not
 *        suitable
 *
 * @return  0: error.
 *         >0: succeed, return the milliseconds
 */
slong_t sk_time_ms();

slong_t sk_time_threadcpu_ns();

#endif

