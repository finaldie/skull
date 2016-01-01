#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "api/sk_utils.h"
#include "api/sk_sched.h"
#include "api/sk_service.h"
#include "api/sk_log.h"
#include "api/sk_log_helper.h"
#include "api/sk_env.h"
#include "api/sk_metrics.h"

#include "api/sk_pto.h"
/**
 * @desc Run api callback
 *
 * @note This method will run in the api caller engine
 */
static
int _run(sk_sched_t* sched, sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(sched);
    SK_ASSERT(entity);
    SK_ASSERT(txn);
    SK_ASSERT(proto_msg);

    // 1. unpack the parameters
    ServiceTaskCb* task_cb_msg = proto_msg;
    uint64_t task_id         = task_cb_msg->task_id;
    const char* service_name = task_cb_msg->service_name;
    const char* api_name     = task_cb_msg->api_name;
    sk_txn_task_status_t task_status = task_cb_msg->task_status;

    // 2. mark the txn task complete
    sk_txn_taskdata_t* taskdata = sk_txn_taskdata(txn, task_id);
    const char* caller_module_name = taskdata->caller_module->name;
    sk_txn_task_setcomplete(txn, task_id, task_status);

    // 3. get the target service
    sk_service_t* service = sk_core_service(SK_ENV_CORE, service_name);
    SK_ASSERT(service);

    // 4. run a specific service api callback
    SK_LOG_SETCOOKIE("module.%s", caller_module_name);
    SK_ENV_POS = SK_ENV_POS_MODULE;

    int ret = sk_service_run_iocall_cb(service, txn, task_id, api_name);

    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    SK_ENV_POS = SK_ENV_POS_CORE;

    if (ret) {
        SK_LOG_ERROR(SK_ENV_LOGGER,
                 "Error in service task callback, ret: %d, task_id: %d\n",
                 ret, task_id);
    }

    // 5. send a complete protocol back to master
    ServiceTaskComplete task_complete_msg = SERVICE_TASK_COMPLETE__INIT;
    task_complete_msg.service_name = (char*) service_name;
    task_complete_msg.resume_wf    = sk_txn_module_complete(txn);

    sk_sched_send(SK_ENV_SCHED, SK_ENV_MASTER_SCHED, entity, txn,
                  SK_PTO_SERVICE_TASK_COMPLETE, &task_complete_msg, 0);

    // 6. log the task lifetime for debugging purpose
    unsigned long long task_lifetime = sk_txn_task_lifetime(txn, task_id);
    SK_LOG_TRACE(SK_ENV_LOGGER, "service: one task id: %d completed, "
                 "cost %llu usec", (int)task_id, task_lifetime);

    return 0;
}

sk_proto_t sk_pto_srv_task_cb = {
    .priority   = SK_PTO_PRI_6,
    .descriptor = &service_task_cb__descriptor,
    .run        = _run
};
