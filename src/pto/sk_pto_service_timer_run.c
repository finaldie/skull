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
          sk_pto_hdr_t* msg)
{
    SK_ASSERT(src == SK_ENV_MASTER_SCHED);
    SK_ASSERT(sk_pto_check(SK_PTO_SVC_TIMER_RUN, msg));

    uint32_t id = SK_PTO_SVC_TIMER_RUN;
    sk_service_t*  service = sk_pto_arg(id, msg, 0)->p;
    sk_service_job job     = sk_pto_arg(id, msg, 1)->f;
    sk_obj_t*      ud      = sk_pto_arg(id, msg, 2)->p;
    int            valid   = sk_pto_arg(id, msg, 3)->b;
    sk_service_job_ret_t status = sk_pto_arg(id, msg, 4)->u32;
    uint32_t       interval = sk_pto_arg(id, msg, 5)->u32;

    sk_print("In timer run: job: %p. interval: %u\n",
             (void*)(intptr_t)job, interval);

    // Run timer job
    SK_LOG_SETCOOKIE("service.%s", sk_service_name(service));
    SK_ENV_POS_SAVE(SK_ENV_POS_SERVICE, service);

    job(service, status, ud, valid);

    SK_ENV_POS_RESTORE();
    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);

    if (status == SK_SRV_JOB_OK) {
        // Notify master the service task has completed
        sk_sched_send(sched, src, entity, NULL, 0, SK_PTO_SVC_TIMER_DONE,
                      service, interval);
    } else {
        // Destory timer entity directly since we won't call timer complete
        if (!interval) {
            sk_entity_safe_destroy(entity);
        }
    }

    return 0;
}

sk_proto_ops_t sk_pto_ops_svc_timer_run = {
    .run = _run
};
