#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "api/sk_entity.h"

void sk_entity_tcp_create(sk_entity_t* entity, void* ud) {
    sk_entity_evb_create(entity, ud);
}
