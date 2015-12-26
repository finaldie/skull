#ifndef SK_ENTITY_UTIL_H
#define SK_ENTITY_UTIL_H

#include "flibs/fev.h"
#include "flibs/fev_buff.h"

#include "api/sk_entity.h"

void sk_entity_util_unpack(fev_state* fev, fev_buff* evbuff,
                           sk_entity_t* entity);

#endif

