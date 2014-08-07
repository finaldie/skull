
#include "api/sk_entity.h"
#include "api/sk_sched.h"
#include "api/sk_pto.h"

static
int _run(sk_sched_t* sched, sk_entity_t* entity, void* proto_msg)
{
    sk_entity_mark_inactive(entity);
    return 0;
}

sk_proto_t sk_pto_net_destroy = {
    .priority = SK_PTO_PRI_9,
    .descriptor = &net_destroy__descriptor,
    .run = _run
};
