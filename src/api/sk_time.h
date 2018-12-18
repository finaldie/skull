#ifndef SK_TIME_H
#define SK_TIME_H

#include <time.h>

#include "api/sk_types.h"

void sk_time_init();

ulong_t sk_time_res();

/**
 * Unit: nanoseconds. clockid = CLOCK_MONOTONIC by default
 *
 * @return  0: error.
 *         >0: succeed, return the nanoseconds
 */
ulong_t sk_time_ns();

/**
 * Unit: milliseconds. clockid = CLOCK_MONOTONIC_COARSE by default
 *
 * @note clockid could be downgraded to CLOCK_MONOTONIC if resolution is not
 *        suitable
 *
 * @return  0: error.
 *         >0: succeed, return the milliseconds
 */
ulong_t sk_time_ms();

#endif

