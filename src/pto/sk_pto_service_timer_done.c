#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"
#include "api/sk_object.h"
#include "api/sk_entity.h"
#include "api/sk_entity_util.h"
#include "api/sk_txn.h"
#include "api/sk_pto.h"
#include "api/sk_metrics.h"
#include "api/sk_service.h"

// This proto is ran in master engine
static
int _run (const sk_sched_t* sched, const sk_sched_t* src, sk_entity_t* entity,
          sk_txn_t* txn, sk_pto_hdr_t* msg)
{
    SK_ASSERT(SK_ENV_ENGINE == SK_ENV_CORE->master);
    SK_ASSERT(sk_pto_check(SK_PTO_SVC_TIMER_DONE, msg));

    uint32_t id = SK_PTO_SVC_TIMER_DONE;
    sk_service_t* service  = sk_pto_arg(id, msg, 0)->p;
    int           status   = sk_pto_arg(id, msg, 1)->i;
    uint32_t      interval = sk_pto_arg(id, msg, 2)->u32;

    // 1. mark task complete
    if (status == SK_SRV_JOB_OK) {
        sk_service_task_setcomplete(service);
    }

    // 2. Destory timer entity if this is not a cron timer entity
    if (!interval) {
        sk_entity_safe_destroy(entity);
    }

    // 3. Update service timer metrics
    if (status == SK_SRV_JOB_OK) {
        sk_metrics_global.srv_timer_complete.inc(1);
        sk_metrics_worker.srv_timer_complete.inc(1);
    } else { // So far, only one reason is possible here
        SK_ASSERT_MSG(status == SK_SRV_JOB_ERROR_BUSY, "status=%d\n", status);
        sk_metrics_worker.srv_timer_busy.inc(1);
        sk_metrics_global.srv_timer_busy.inc(1);
    }

    // 4. Re-schedule tasks
    sk_service_schedule_tasks(service);
    return 0;
}

sk_proto_ops_t sk_pto_ops_svc_timer_done = {
    .run = _run
};
