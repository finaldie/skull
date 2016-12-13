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
 * @desc invoke a service io call
 *
 * @note This method will run in the master thread
 */
static
int _run(const sk_sched_t* sched, const sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(sched);
    SK_ASSERT(entity);
    SK_ASSERT(txn);
    SK_ASSERT(proto_msg);
    SK_ASSERT(SK_ENV_ENGINE == SK_ENV_CORE->master);

    // 1. unpack the parameters
    ServiceIocall* iocall_msg = proto_msg;
    uint64_t    task_id       = iocall_msg->task_id;
    const char* service_name  = iocall_msg->service_name;
    const char* api_name      = iocall_msg->api_name;
    int         bidx          = iocall_msg->bio_idx;
    sk_txn_taskdata_t* taskdata = (sk_txn_taskdata_t*) (uintptr_t) iocall_msg->txn_task;

    // 2. find the target service
    sk_service_t* service = sk_core_service(SK_ENV_CORE, service_name);
    SK_ASSERT_MSG(service, "service_name: %s\n", service_name);

    const sk_service_cfg_t* srv_cfg = sk_service_config(service);
    const sk_service_api_t* srv_api = sk_service_api(service, api_name);
    SK_ASSERT(srv_cfg && srv_api);

    // 3. queue a specific service call
    // 3.1 construct the service task
    sk_srv_task_t task;
    memset(&task, 0, sizeof(task));

    SK_ASSERT(srv_api->type == SK_SRV_API_READ ||
              srv_api->type == SK_SRV_API_WRITE);

    if (srv_api->type == SK_SRV_API_READ) {
        task.base.type = SK_QUEUE_ELEM_READ;
    } else {
        task.base.type = SK_QUEUE_ELEM_WRITE;
    }

    task.type      = SK_SRV_TASK_API_QUERY;
    task.io_status = SK_SRV_IO_STATUS_OK;
    task.bidx      = bidx;
    task.src       = src;
    task.data.api.service = service;
    task.data.api.txn     = txn;
    task.data.api.name    = srv_api->name;
    task.data.api.task_id = task_id;
    task.data.api.txn_task = taskdata;

    // 3.2 push task to service
    int ret = 0;
    sk_srv_status_t srv_status = sk_service_push_task(service, &task);
    if (srv_status == SK_SRV_STATUS_BUSY) {
        task.io_status = SK_SRV_IO_STATUS_BUSY;
    }

    // 3.3 If push error, schedule back the task directly with error status
    if (task.io_status == SK_SRV_IO_STATUS_BUSY) {
        sk_metrics_worker.srv_iocall_busy.inc(1);
        sk_metrics_global.srv_iocall_busy.inc(1);
        SK_LOG_DEBUG(SK_ENV_LOGGER, "ServiceIocall Busy, service: %s, "
                    "api: %s", service_name, srv_api->name);

        sk_service_schedule_task(service, &task);
        ret = 1;
        goto io_call_exit;
    }

    // 4. schedule 'service task' according the data mode
    //    (as much as possible) and deliver them to worker thread
    size_t scheduled_task = sk_service_schedule_tasks(service);
    SK_LOG_TRACE(SK_ENV_LOGGER, "Service Iocall:, service name %s, "
                 "scheduled %zu tasks", service_name, scheduled_task);

    // 5. update metrics
    sk_metrics_worker.srv_iocall.inc(1);
    sk_metrics_global.srv_iocall.inc(1);

io_call_exit:
    return ret;
}

sk_proto_opt_t sk_pto_srv_iocall = {
    .descriptor = &service_iocall__descriptor,
    .run        = _run
};
