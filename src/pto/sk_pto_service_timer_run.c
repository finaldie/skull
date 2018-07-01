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
#include "api/sk_log_helper.h"
#include "api/sk_service.h"

// This proto is ran in the caller engine or bio
static
int _run (const sk_sched_t* sched, const sk_sched_t* src /*master*/,
          sk_entity_t* entity, sk_txn_t* txn,
          void* proto_msg)
{
    SK_ASSERT(src == SK_ENV_MASTER_SCHED);

    ServiceTimerRun* msg = proto_msg;
    sk_service_t*  service = (sk_service_t*) (uintptr_t) msg->svc;
    sk_service_job job     = *(sk_service_job*) msg->job.data;
    sk_obj_t*      ud      = (sk_obj_t*) (uintptr_t) msg->ud;
    int            valid   = msg->valid;
    sk_service_job_ret_t status = (sk_service_job_ret_t)msg->status;

    // Run timer job
    SK_LOG_SETCOOKIE("service.%s", sk_service_name(service));
    SK_ENV_POS_SAVE(SK_ENV_POS_SERVICE, service);

    job(service, status, ud, valid);

    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    SK_ENV_POS_RESTORE();

    if (status == SK_SRV_JOB_OK) {
        // Notify master the service task has completed
        ServiceTimerComplete complete_msg = SERVICE_TIMER_COMPLETE__INIT;
        complete_msg.svc = (uintptr_t) (void*) service;
        sk_sched_send(sched, src, entity, NULL, SK_PTO_SVC_TIMER_COMPLETE,
                      &complete_msg, 0);
    } else {
        // Destory timer entity directly since we won't call timer complete
        sk_entity_safe_destroy(entity);
    }

    return 0;
}

sk_proto_opt_t sk_pto_svc_timer_run = {
    .descriptor = &service_timer_run__descriptor,
    .run        = _run
};
