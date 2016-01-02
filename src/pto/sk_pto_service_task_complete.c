#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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
int _run(sk_sched_t* sched, sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(sched);
    SK_ASSERT(entity);
    SK_ASSERT(txn);
    SK_ASSERT(proto_msg);
    SK_ASSERT(SK_ENV_ENGINE->type == SK_ENGINE_MASTER);

    // 1. unpack the parameters
    ServiceTaskComplete* task_complete_msg = proto_msg;
    const char* service_name = task_complete_msg->service_name;
    int resume_wf = task_complete_msg->resume_wf;

    // 2. find the target service
    sk_service_t* service = sk_core_service(SK_ENV_CORE, service_name);
    SK_ASSERT(service);

    // 3. do some updating job for this service
    sk_service_task_setcomplete(service);

    // 4. Resume txn workflow if needed
    if (resume_wf) {
        sk_sched_send(sched, src, entity, txn, SK_PTO_WORKFLOW_RUN, NULL, 0);
    }

    // 5. schedule another round of service call if exist
    size_t scheduled_task = sk_service_schedule_tasks(service);
    SK_LOG_TRACE(SK_ENV_LOGGER, "Service Iocall:, service name %s, "
                 "scheduled %zu tasks", service_name, scheduled_task);

    // 6. update metrics
    sk_metrics_worker.srv_iocall_complete.inc(1);
    sk_metrics_global.srv_iocall_complete.inc(1);

    return 0;
}

sk_proto_opt_t sk_pto_srv_task_complete = {
    .descriptor = &service_task_complete__descriptor,
    .run        = _run
};
