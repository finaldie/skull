#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "api/sk_utils.h"
#include "api/sk_sched.h"
#include "api/sk_service.h"
#include "api/sk_log.h"
#include "api/sk_env.h"
#include "api/sk_metrics.h"

#include "api/sk_pto.h"

/**
 * @desc Notify the service a task has completed
 *
 * @note This method will run in the master
 */
static
int _run(const sk_sched_t* sched, const sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, sk_pto_hdr_t* msg)
{
    SK_ASSERT(sched);
    SK_ASSERT(entity);
    SK_ASSERT(txn);
    SK_ASSERT(msg);
    SK_ASSERT(sk_pto_check(SK_PTO_SVC_TASK_DONE, msg));
    SK_ASSERT(SK_ENV_ENGINE->type == SK_ENGINE_MASTER);

    // 1. unpack the parameters
    const char* service_name = sk_pto_arg(SK_PTO_SVC_TASK_DONE, msg, 0)->s;
    bool resume_wf           = sk_pto_arg(SK_PTO_SVC_TASK_DONE, msg, 1)->b;
    bool svc_task_done       = sk_pto_arg(SK_PTO_SVC_TASK_DONE, msg, 2)->b;
    sk_print("Debug: svc_task_done: %d\n", svc_task_done);

    // 2. find the target service
    sk_service_t* service = sk_core_service(SK_ENV_CORE, service_name);
    SK_ASSERT(service);

    // 3. do some updating job for this service if necessary
    if (svc_task_done) {
        sk_service_task_setcomplete(service);

        // update metrics
        sk_metrics_worker.srv_iocall_complete.inc(1);
        sk_metrics_global.srv_iocall_complete.inc(1);
    }

    // 4. Resume txn workflow if needed
    if (resume_wf) {
        sk_print("Resume workflow\n");
        SK_LOG_TRACE(SK_ENV_LOGGER, "Resume workflow");
        sk_sched_send(sched, src, entity, txn, 0, SK_PTO_WORKFLOW_RUN);
    }

    // 5. schedule another round of service call if exist
    size_t scheduled_task = sk_service_schedule_tasks(service);
    SK_LOG_TRACE(SK_ENV_LOGGER, "Service Iocall:, service name %s, "
                 "scheduled %zu tasks", service_name, scheduled_task);

    return 0;
}

sk_proto_ops_t sk_pto_ops_srv_task_done = {
    .run = _run
};
