
#include "api/sk_utils.h"
#include "api/sk_entity.h"
#include "api/sk_sched.h"
#include "api/sk_pto.h"

static
int _run(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(entity);
    sk_entity_mark(entity, SK_ENTITY_INACTIVE);
    sk_print("mark entity as inactive\n");
    return 0;
}

sk_proto_t sk_pto_net_destroy = {
    .priority = SK_PTO_PRI_9,
    .descriptor = &net_destroy__descriptor,
    .run = _run
};
