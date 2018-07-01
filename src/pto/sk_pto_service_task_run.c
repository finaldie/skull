#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "api/sk_const.h"
#include "api/sk_utils.h"
#include "api/sk_sched.h"
#include "api/sk_service.h"
#include "api/sk_log.h"
#include "api/sk_log_helper.h"
#include "api/sk_env.h"
#include "api/sk_metrics.h"

#include "api/sk_pto.h"

/**
 * @desc Run a service task
 *
 * @note This method will run in the worker/bio thread
 */
static
int _run(const sk_sched_t* sched, const sk_sched_t* src /*master*/,
         sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(sched);
    SK_ASSERT(entity);
    SK_ASSERT(txn);
    SK_ASSERT(proto_msg);
    SK_ASSERT(src == SK_ENV_MASTER_SCHED);

    // 1. unpack the parameters
    ServiceTaskRun* task_run_msg = proto_msg;
    const char* service_name = task_run_msg->service_name;
    const char* api_name     = task_run_msg->api_name;
    uint32_t    io_status    = task_run_msg->io_status;
    sk_sched_t* api_caller   = (sk_sched_t*) (uintptr_t) task_run_msg->src;
    sk_txn_taskdata_t* taskdata = (sk_txn_taskdata_t*) (uintptr_t) task_run_msg->taskdata;
    SK_ASSERT(io_status < SK_SRV_IO_STATUS_MAX);

    sk_srv_status_t srv_status = SK_SRV_STATUS_OK;
    int ret = 0;

    // 2. get the target service
    sk_service_t* service = sk_core_service(SK_ENV_CORE, service_name);
    SK_ASSERT(service);

    // 3. run a specific service call
    // notes: even the user iocall failed, still need to send back a
    //        'task_complete' message, or the user module will be hanged

    SK_LOG_SETCOOKIE("service.%s", service_name);
    SK_ENV_POS_SAVE(SK_ENV_POS_SERVICE, service);

    srv_status =
        sk_service_run_iocall(service, txn, taskdata, api_name, io_status);
    SK_ASSERT(srv_status == SK_SRV_STATUS_OK);

    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    SK_ENV_POS_RESTORE();

    if (srv_status != SK_SRV_STATUS_OK) {
        SK_LOG_ERROR(SK_ENV_LOGGER, "service: user io call failed \
                     service_name: %s, api_name: %s", service_name, api_name);
        ret = 1;
    }

    // 4. check task done or not
    int task_done = sk_txn_task_done(taskdata);
    sk_txn_task_status_t txn_status =
        task_done ? SK_TXN_TASK_DONE : SK_TXN_TASK_PENDING;

    // 5. schedule it back and run api callback
    ServiceTaskCb task_cb_msg = SERVICE_TASK_CB__INIT;
    task_cb_msg.taskdata      = (uint64_t) (uintptr_t) taskdata;
    task_cb_msg.service_name  = (char*) service_name;
    task_cb_msg.api_name = (char*) sk_service_api(service, api_name)->name;
    task_cb_msg.task_status   = (uint32_t) txn_status;
    task_cb_msg.svc_task_done = 1;

    sk_sched_send(SK_ENV_SCHED, api_caller, entity, txn,
                  SK_PTO_SVC_TASK_CB, &task_cb_msg, 0);

    // 7. update metrics
    sk_metrics_worker.srv_iocall_execute.inc(1);
    sk_metrics_global.srv_iocall_execute.inc(1);

    return ret;
}

sk_proto_opt_t sk_pto_srv_task_run = {
    .descriptor = &service_task_run__descriptor,
    .run        = _run
};
