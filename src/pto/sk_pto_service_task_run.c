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
 * @note This method will run in the worker thread
 */
static
int _run(sk_sched_t* sched, sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(sched);
    SK_ASSERT(entity);
    SK_ASSERT(txn);
    SK_ASSERT(proto_msg);
    SK_ASSERT(src == SK_ENV_CORE->master->sched);

    // 1. unpack the parameters
    ServiceTaskRun* task_run_msg = proto_msg;
    uint64_t task_id         = task_run_msg->task_id;
    const char* service_name = task_run_msg->service_name;
    const char* api_name     = task_run_msg->api_name;
    uint32_t io_status       = task_run_msg->io_status;
    SK_ASSERT(io_status < SK_SRV_IO_STATUS_MAX);

    sk_srv_status_t srv_status = SK_SRV_STATUS_OK;
    int ret = 0;

    // 2. find the target service
    sk_service_t* service = sk_core_service(SK_ENV_CORE, service_name);
    SK_ASSERT(service);

    // 3. run a specific service call
    // notes: even the user iocall failed, still need to send back a
    //        'task_complete' message, or the user module will be hanged

    SK_LOG_SETCOOKIE("service.%s", service_name);
    sk_txn_setpos(txn, SK_TXN_POS_SERVICE);

    srv_status = sk_service_run_iocall(service, txn, task_id,
                                       api_name, io_status);
    if (srv_status != SK_SRV_STATUS_OK) {
        SK_LOG_ERROR(SK_ENV_LOGGER, "service: user io call failed \
                     service_name: %s, api_name: %s", service_name, api_name);
        ret = 1;
    }

    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    sk_txn_setpos(txn, SK_TXN_POS_CORE);

    // 4. mark the txn task complete
    sk_txn_task_status_t txn_status = srv_status == SK_SRV_STATUS_OK
                                        ? SK_TXN_TASK_DONE
                                        : SK_TXN_TASK_ERROR;
    sk_txn_task_setcomplete(txn, task_id, txn_status);

    // 5. send a complete protocol back to master
    ServiceTaskComplete task_complete_msg = SERVICE_TASK_COMPLETE__INIT;
    task_complete_msg.task_id = task_id;
    task_complete_msg.service_name = (char*) service_name;

    sk_sched_send(SK_ENV_SCHED, src, entity, txn,
                  SK_PTO_SERVICE_TASK_COMPLETE, &task_complete_msg, 0);

    // 6. update metrics
    sk_metrics_worker.srv_iocall_execute.inc(1);
    sk_metrics_global.srv_iocall_execute.inc(1);

    return ret;
}

sk_proto_t sk_pto_srv_task_run = {
    .priority = SK_PTO_PRI_6,
    .descriptor = &service_task_run__descriptor,
    .run = _run
};
