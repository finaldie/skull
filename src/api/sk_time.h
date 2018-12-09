#ifndef SK_TIME_H
#define SK_TIME_H

#include <time.h>

#include "api/sk_types.h"

/**
 * Unit: nanoseconds. clockid = CLOCK_MONOTONIC by default
 *
 * @return  0: error.
 *         >0: succeed, return the nanoseconds
 */
ulong_t sk_gettime();

#endif

