#ifndef SK_TRIGGER_UTILS_H
#define SK_TRIGGER_UTILS_H

#include "api/sk_sched.h"
#include "api/sk_entity.h"

ssize_t sk_trigger_util_unpack(sk_entity_t* entity, const sk_sched_t* deliver_to);

#endif

