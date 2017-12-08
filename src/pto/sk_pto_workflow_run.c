#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "api/sk_pto.h"
#include "api/sk_txn.h"
#include "api/sk_log.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"
#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_event.h"
#include "api/sk_metrics.h"
#include "api/sk_log_helper.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_entity_util.h"
#include "api/sk_driver_utils.h"

static int _run(const sk_sched_t* sched, const sk_sched_t* src,
                sk_entity_t* entity, sk_txn_t* txn,
                void* proto_msg);

static
int _module_run(const sk_sched_t* sched, const sk_sched_t* src,
                sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    unsigned long long start_time = sk_txn_alivetime(txn);

    // 1. Run the next module
    sk_module_t* module = sk_txn_next_module(txn);
    if (!module) {
        SK_LOG_TRACE(SK_ENV_LOGGER, "no next module in this workflow, stop it");
        sk_txn_setstate(txn, SK_TXN_COMPLETED);
        return _run(sched, src, entity, txn, proto_msg);
    }

    // Before run module, set the module name for this module
    // NOTES: the cookie have 256 bytes limitation
    const char* module_name = module->cfg->name;
    SK_LOG_SETCOOKIE("module.%s", module_name);
    SK_ENV_POS = SK_ENV_POS_MODULE;

    // Run the module
    int ret = module->run(module->md, txn);
    sk_print("module execution return code=%d\n", ret);
    sk_module_stat_inc_run(module);

    unsigned long long alivetime = sk_txn_alivetime(txn);
    sk_txn_log_add(txn, "-> m:%s:run start: %llu end: %llu ",
                   module_name, start_time, alivetime);

    // After module exit, set back the module name
    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    SK_ENV_POS = SK_ENV_POS_CORE;

    if (ret) {
        sk_print("module (%s) error occurred\n", module_name);
        SK_LOG_DEBUG(SK_ENV_LOGGER,
            "module (%s) error occurred", module_name);

        sk_txn_setstate(txn, SK_TXN_ERROR);
    }

    // 2. Check whether the module is completed, if not, means there are
    //    some service io calls on the fly. When all the service io calls
    //    completed, the master will re-schedule the 'workflow_run' protocol
    //    again
    if (!sk_txn_module_complete(txn)) {
        if (!ret) {
            sk_txn_setstate(txn, SK_TXN_PENDING);
        }

        sk_print("txn pending, waiting for service io calls, module %s\n",
                 module_name);
        SK_LOG_TRACE(SK_ENV_LOGGER, "txn pending, waiting for service calls, "
                     "module %s", module_name);
        return 0;
    }

    if (ret) {
        sk_print("module (%s) error occurred, goto pack directly\n",
                 module_name);

        SK_LOG_DEBUG(SK_ENV_LOGGER,
            "module (%s) error occurred, goto pack directly", module_name);
        return _run(sched, src, entity, txn, proto_msg);
    }

    // 3. Check whether is the last module:
    if (!sk_txn_is_last_module(txn)) {
        // 3.1 If no, schedule to next module
        sk_print("Doesn't reach the last module, schedule to next module\n");
        sk_sched_send(sched, sched, entity, txn, SK_PTO_WORKFLOW_RUN, NULL, 0);
        return 0;
    } else {
        // 3.2 Set complete state
        sk_txn_setstate(txn, SK_TXN_COMPLETED);
        return _run(sched, src, entity, txn, proto_msg);
    }
}

static
void _write_txn_log(sk_txn_t* txn) {
    unsigned long long alivetime = sk_txn_alivetime(txn);
    bool slowlog = false;
    bool txnlog  = false;

    if (SK_ENV_CONFIG->slowlog_ms > 0) {
        if (alivetime >= (unsigned long long)SK_ENV_CONFIG->slowlog_ms) {
            slowlog = true;
        }
    } else if (SK_ENV_CONFIG->txn_logging) {
        txnlog = true;
    }

    if (!slowlog && !txnlog) {
        return;
    }

    // 1. Push a null-str
    sk_txn_log_end(txn);

    // 2. Log the full txn log

    if (txnlog) {
        SK_LOG_INFO(SK_ENV_LOGGER, "TxnLog: status: %d duration: %.3f ms | %s",
            sk_txn_error(txn), (double)alivetime / 1000, sk_txn_log(txn));
    }

    if (slowlog) {
        SK_LOG_INFO(SK_ENV_LOGGER, "SlowLog: status: %d duration: %.3f ms | %s",
            sk_txn_error(txn), (double)alivetime / 1000, sk_txn_log(txn));
    }
}

static
void _txn_log_and_destroy(const sk_sched_t* sched, sk_txn_t* txn) {
    sk_entity_t* entity = sk_txn_entity(txn);

    // 1. Log and try to destroy txn
    if (sk_txn_alltask_complete(txn)) {
        _write_txn_log(txn);

        // Trigger next txn in entity iqueue if possible (deliver to current
        //  scheduler
        sk_print("Try to delivery Txns from input queue\n");
        sk_driver_util_deliver(entity, sched, 1);
    }

    int r = sk_txn_safe_destroy(txn);

    // 2. Destroy entity if its flags contain SK_ENTITY_F_DESTROY_NOTXN
    if (r) return;

    int txncnt = sk_entity_taskcnt(entity);
    int eflags = sk_entity_flags(entity);
    sk_entity_status_t st = sk_entity_status(entity);

    if ((st != SK_ENTITY_ACTIVE || eflags & SK_ENTITY_F_DESTROY_NOTXN)
        && txncnt == 0) {
        sk_entity_safe_destroy(entity);
    }
}

static
int _module_pack(const sk_sched_t* sched, const sk_sched_t* src,
                 sk_entity_t* entity, sk_txn_t* txn,
                 void* proto_msg)
{
    sk_workflow_t* workflow = sk_txn_workflow(txn);
    sk_module_t* last_module = sk_workflow_last_module(workflow);
    unsigned long long start_time = sk_txn_alivetime(txn);

    // 1. No pack function means no need to send response
    if (!last_module->pack) {
        sk_txn_setstate(txn, SK_TXN_PACKED);
        return _run(sched, src, entity, txn, proto_msg);
    }

    // 2. Pack the data, and send the response if needed
    const char* module_name = last_module->cfg->name;
    SK_LOG_SETCOOKIE("module.%s", module_name);

    int ret = last_module->pack(last_module->md, txn);
    if (ret) { // Error occured
        // Here cannot set state to ERROR, which will cause the infinite-loop
        sk_entity_mark(entity, SK_ENTITY_INACTIVE);
    }

    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    sk_module_stat_inc_pack(last_module);

    size_t packed_data_sz = 0;
    const char* packed_data = sk_txn_output(txn, &packed_data_sz);
    sk_print("module packed data size=%zu\n", packed_data_sz);

    if (!packed_data || !packed_data_sz) {
        sk_print("module no need to send response\n");
    } else {
        sk_print("write data sz:%zu\n", packed_data_sz);
        sk_entity_write(entity, packed_data, packed_data_sz);

        // Record metrics
        sk_metrics_worker.response.inc(1);
        sk_metrics_global.response.inc(1);
    }

    // 3. Increase metrics and set state
    unsigned long long alivetime = sk_txn_alivetime(txn);
    sk_print("txn time: %llu\n", alivetime);
    sk_metrics_worker.latency.inc((uint32_t)alivetime);
    sk_metrics_global.latency.inc((uint32_t)alivetime);

    sk_txn_log_add(txn, "-> m:%s:pack start: %llu end: %llu",
                   module_name, start_time, alivetime);

    sk_txn_setstate(txn, SK_TXN_PACKED);
    return _run(sched, src, entity, txn, proto_msg);
}

// Every time pick up the next module in the workflow module list, then execute
// the its `run` method. If reach the last module of the workflow, then will
// execute the `pack` method
static
int _run(const sk_sched_t* sched, const sk_sched_t* src, sk_entity_t* entity,
         sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(sched && entity && txn);
    sk_txn_state_t state = sk_txn_state(txn);

    // Check whether timeout
    if (sk_txn_timeout(txn)) {
        sk_txn_setstate(txn, SK_TXN_TIMEOUT);
    }

    switch (state) {
    case SK_TXN_INIT: {
        sk_print("txn - INIT\n");
        //sk_txn_setstate(txn, SK_TXN_UNPACKED);
        sk_txn_setstate(txn, SK_TXN_RUNNING);
        return _run(sched, src, entity, txn, proto_msg);
    }
    case SK_TXN_UNPACKED: {
        sk_print("txn - UNPACKED\n");
        sk_txn_setstate(txn, SK_TXN_RUNNING);
        return _run(sched, src, entity, txn, proto_msg);
    }
    case SK_TXN_RUNNING: {
        sk_print("txn - RUNNING\n");
        return _module_run(sched, src, entity, txn, proto_msg);
    }
    case SK_TXN_PENDING: {
        sk_print("txn - PENDING\n");
        sk_txn_setstate(txn, SK_TXN_RUNNING);
        return _run(sched, src, entity, txn, proto_msg);
    }
    case SK_TXN_COMPLETED: {
        sk_print("txn - COMPLETED\n");
        return _module_pack(sched, src, entity, txn, proto_msg);
    }
    case SK_TXN_PACKED: {
        sk_print("txn - PACKED: txn destroy\n");
        sk_txn_setstate(txn, SK_TXN_DESTROYED);
        _txn_log_and_destroy(sched, txn);
        break;
    }
    case SK_TXN_ERROR:
    case SK_TXN_TIMEOUT: {
        sk_print("txn - ERROR or TIMEOUT: txn error or timeout\n");
        return _module_pack(sched, src, entity, txn, proto_msg);
    }
    case SK_TXN_DESTROYED: {
        sk_print("txn - DESTROYED: txn destroy\n");
        _txn_log_and_destroy(sched, txn);
        break;
    }
    default:
        sk_print("Unexpect txn state: %d, ignored\n", state);
        SK_LOG_FATAL(SK_ENV_LOGGER, "Unexpect txn state: %d, exit", state);
        SK_ASSERT(0);
        break;
    }

    return 0;
}

sk_proto_opt_t sk_pto_workflow_run = {
    .descriptor = &workflow_run__descriptor,
    .run        = _run
};
