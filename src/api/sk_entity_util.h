#ifndef SK_ENTITY_UTIL_H
#define SK_ENTITY_UTIL_H

#include "flibs/fev.h"
#include "flibs/fev_buff.h"

#include "api/sk_entity.h"
#include "api/sk_sched.h"

void sk_entity_util_unpack(fev_state* fev, fev_buff* evbuff,
                           sk_entity_t* entity);

// return 0: destroyed immediately
// return 1: delivery to its owner to destroy, need some time
int sk_entity_safe_destroy(sk_entity_t*);

sk_sched_t* sk_entity_sched(const sk_entity_t*);

#endif

