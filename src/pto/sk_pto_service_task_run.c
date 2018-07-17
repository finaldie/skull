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
         sk_entity_t* entity, sk_txn_t* txn, sk_pto_hdr_t* msg)
{
    SK_ASSERT(sched);
    SK_ASSERT(entity);
    SK_ASSERT(txn);
    SK_ASSERT(msg);
    SK_ASSERT(sk_pto_check(SK_PTO_SVC_TASK_RUN, msg));
    SK_ASSERT(src == SK_ENV_MASTER_SCHED);

    // 1. unpack the parameters
    uint32_t id = SK_PTO_SVC_TASK_RUN;
    const char* service_name = sk_pto_arg(id, msg, 0)->s;
    const char* api_name     = sk_pto_arg(id, msg, 1)->s;
    uint32_t    io_status    = sk_pto_arg(id, msg, 2)->u32;
    sk_sched_t* api_caller   = sk_pto_arg(id, msg, 3)->p;
    sk_txn_taskdata_t* taskdata = sk_pto_arg(id, msg, 4)->p;;
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

    SK_ENV_POS_RESTORE();
    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);

    if (srv_status != SK_SRV_STATUS_OK) {
        SK_LOG_ERROR(SK_ENV_LOGGER, "service: user io call failed \
                     service_name: %s, api_name: %s", service_name, api_name);
        ret = 1;
    }

    // 4. check task done or not
    int task_done = sk_txn_task_done(taskdata);
    sk_txn_task_status_t txn_status =
        task_done ? SK_TXN_TASK_DONE : SK_TXN_TASK_PENDING;

    bool svc_task_done = true;

    // 5. schedule it back and run api callback

    sk_sched_send(SK_ENV_SCHED, api_caller, entity, txn, 0,
                  SK_PTO_SVC_TASK_CB, taskdata, sk_service_name(service),
                  sk_service_api(service, api_name)->name, txn_status,
                  svc_task_done);

    // 7. update metrics
    sk_metrics_worker.srv_iocall_execute.inc(1);
    sk_metrics_global.srv_iocall_execute.inc(1);

    return ret;
}

sk_proto_ops_t sk_pto_ops_srv_task_run = {
    .run        = _run
};
