#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"
#include "api/sk_object.h"
#include "api/sk_entity.h"
#include "api/sk_txn.h"
#include "api/sk_pto.h"
#include "api/sk_service.h"

// This proto is ran in master engine
static
int _run (sk_sched_t* sched, sk_sched_t* src, sk_entity_t* entity, sk_txn_t* txn,
          void* proto_msg)
{
    SK_ASSERT(SK_ENV_ENGINE == SK_ENV_CORE->master);

    ServiceTimerComplete* complete_msg = proto_msg;
    sk_service_t* service = (sk_service_t*) (uintptr_t) complete_msg->svc;

    // 1. mark task complete
    sk_service_task_setcomplete(service);

    // 2. Re-schedule tasks
    sk_service_schedule_tasks(service);

    // 3. Destory timer entity
    sk_sched_send(sched, src, entity, NULL,
                  SK_PTO_ENTITY_DESTROY, NULL, 0);
    return 0;
}

sk_proto_t sk_pto_svc_timer_complete = {
    .priority = SK_PTO_PRI_9,
    .descriptor = &service_timer_complete__descriptor,
    .run = _run
};
