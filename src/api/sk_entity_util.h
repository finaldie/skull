#ifndef SK_ENTITY_UTIL_H
#define SK_ENTITY_UTIL_H

#include "api/sk_entity.h"
#include "api/sk_sched.h"

// return 0: destroyed immediately
// return 1: delivery to its owner to destroy, need some time
int sk_entity_safe_destroy(sk_entity_t*);

sk_sched_t* sk_entity_sched(const sk_entity_t*);

#endif

