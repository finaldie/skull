#ifndef SK_DRIVER_UTILS_H
#define SK_DRIVER_UTILS_H

#include "api/sk_sched.h"
#include "api/sk_entity.h"

/**
 * Return - >0 unpack done, msg can be deliver
 *        - =0 still need more data
 *        - <0 error occurred
 */
ssize_t sk_driver_util_unpack(sk_entity_t* entity);

/**
 * Return how many TXNs be delivered
 */
int sk_driver_util_deliver(sk_entity_t*, const sk_sched_t* deliver_to, int max);

#endif

