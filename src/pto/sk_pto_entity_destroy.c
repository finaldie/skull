#include <stdlib.h>

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_entity.h"
#include "api/sk_sched.h"
#include "api/sk_pto.h"

static
int _run(sk_sched_t* sched, sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(entity);
    sk_print("entity status=%d, will be deleted\n",
                 sk_entity_status(entity));

    sk_entity_mark(entity, SK_ENTITY_INACTIVE);
    sk_entity_mgr_del(sk_entity_owner(entity), entity);

    sk_print("mark entity as inactive, %p\n", (void*)entity);
    return 0;
}

sk_proto_t sk_pto_entity_destroy = {
    .priority   = SK_PTO_PRI_9,
    .descriptor = &entity_destroy__descriptor,
    .run        = _run
};
