#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "api/sk_utils.h"
#include "api/sk_sched.h"
#include "api/sk_service.h"
#include "api/sk_log.h"
#include "api/sk_log_helper.h"
#include "api/sk_env.h"
#include "api/sk_entity_util.h"
#include "api/sk_metrics.h"

#include "api/sk_pto.h"
/**
 * @desc Run api callback
 *
 * @note This method will run in the api caller engine (worker)
 */
static
int _run(const sk_sched_t* sched, const sk_sched_t* src,
         sk_entity_t* entity, sk_txn_t* txn, sk_pto_hdr_t* msg)
{
    SK_ASSERT(sched);
    SK_ASSERT(entity);
    SK_ASSERT(txn);
    SK_ASSERT(msg);
    SK_ASSERT(sk_pto_check(SK_PTO_SVC_TASK_CB, msg));
    SK_ASSERT(sched == sk_entity_sched(entity));

    // 1. unpack the parameters
    uint32_t id = SK_PTO_SVC_TASK_CB;

    sk_txn_taskdata_t* taskdata = sk_pto_arg(id, msg, 0)->p;
    const char* service_name    = sk_pto_arg(id, msg, 1)->s;
    const char* api_name        = sk_pto_arg(id, msg, 2)->s;
    sk_txn_task_status_t task_status =
        (sk_txn_task_status_t)sk_pto_arg(id, msg, 3)->i;
    int svc_task_done           = sk_pto_arg(id, msg, 4)->i;

    sk_module_t* caller_module      = taskdata->caller_module;
    const char*  caller_module_name = caller_module->cfg->name;
    uint64_t task_id = sk_txn_task_id(taskdata->owner);

    // 2. mark the txn task complete
    if (task_status == SK_TXN_TASK_DONE || task_status == SK_TXN_TASK_BUSY) {
        sk_txn_task_setcomplete(txn, task_id, task_status);

        // 3. get the target service
        sk_service_t* service = sk_core_service(SK_ENV_CORE, service_name);
        SK_ASSERT(service);

        // 4. run a specific service api callback
        int ret = 0;
        SK_LOG_SETCOOKIE("module.%s", caller_module_name);
        SK_ENV_POS_SAVE(SK_ENV_POS_MODULE, caller_module);

        ret = sk_service_run_iocall_cb(service, txn, task_id, api_name);

        SK_ENV_POS_RESTORE();
        SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);

        if (ret) {
            SK_LOG_TRACE(SK_ENV_LOGGER,
                "Error in service task callback, module: %s ret: %d, task_id: %d\n",
                caller_module_name, ret, task_id);

            // Mark txn as ERROR, then after iocall complete, the workflow will
            //  be go to 'pack' directly
            sk_txn_setstate(txn, SK_TXN_ERROR);
        }

        slong_t txn_starttime = sk_txn_starttime(txn);
        slong_t txn_alivetime = sk_txn_alivetime(txn);
        slong_t task_starttime = sk_txn_task_starttime(txn, task_id);

        sk_txn_log_add(txn, "; t:%s:%s st: %d cb_st: %d start: %ld end: %ld ",
                       service_name, api_name, task_status, ret,
                       task_starttime - txn_starttime, txn_alivetime);
    }

    // 5. log the task lifetime for debugging purpose
    SK_LOG_TRACE(SK_ENV_LOGGER, "service: one task id: %d completed, "
                 "cost %ld usec", (int)task_id,
                 sk_txn_task_lifetime(txn, task_id));

    // 6. send a complete protocol back to master
    bool resume_wf    = taskdata->cb
        ? sk_txn_module_complete(txn)
        : sk_txn_alltask_complete(txn);

    sk_sched_send(SK_ENV_SCHED, SK_ENV_MASTER_SCHED, entity, txn, 0,
                  SK_PTO_SVC_TASK_DONE, service_name, resume_wf, svc_task_done);
    return 0;
}

sk_proto_ops_t sk_pto_ops_srv_task_cb = {
    .run = _run
};
