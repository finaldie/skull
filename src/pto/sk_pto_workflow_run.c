#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "flibs/fev_buff.h"
#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_event.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_pto.h"
#include "api/sk_txn.h"
#include "api/sk_log.h"
#include "api/sk_env.h"
#include "api/sk_metrics.h"
#include "api/sk_sched.h"

#define SK_MAX_COOKIE_LEN 256

static int _run(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn,
                void* proto_msg);

static
int _module_run(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn,
                void* proto_msg)
{
    // 1. run the next module
    sk_module_t* module = sk_txn_next_module(txn);
    if (!module) {
        SK_LOG_ERROR(SK_ENV_LOGGER, "no module in this workflow, skip it");
        sk_txn_setstate(txn, SK_TXN_COMPLETED);
        return _run(sched, entity, txn, proto_msg);
    }

    // before run module, set the module name for this module
    // NOTES: the cookie have 256 bytes limitation
    sk_logger_setcookie("module.%s", module->name);
    sk_txn_setpos(txn, SK_TXN_POS_MODULE);

    // Run the module
    int ret = module->run(module->md, txn);
    sk_print("module execution return code=%d\n", ret);

    // after module exit, set back the module name
    sk_logger_setcookie(SK_CORE_LOG_COOKIE);
    sk_txn_setpos(txn, SK_TXN_POS_CORE);

    if (ret) {
        SK_ASSERT_MSG(!ret, "un-implemented code path\n");
        return 1;
    }

    // 2. Check whether the module is completed, if not, means there are
    //    some service io calls on the fly. When all the service io calls
    //    completed, the master will re-schedule the 'workflow_run' protocol
    //    again
    if (!sk_txn_module_complete(txn)) {
        sk_txn_setstate(txn, SK_TXN_PENDING);

        sk_print("txn pending, waiting for service io calls, module %s\n",
                 module->name);
        SK_LOG_DEBUG(SK_ENV_LOGGER, "txn pending, waiting for service calls, "
                     "module %s", module->name);
        return 0;
    }

    // 3. check whether is the last module:
    // 3.1 if no, send a event for next module
    if (!sk_txn_is_last_module(txn)) {
        sk_print("doesn't reach the last module\n");
        sk_sched_push(sched, entity, txn, SK_PTO_WORKFLOW_RUN, NULL);
        return 0;
    } else {
        sk_txn_setstate(txn, SK_TXN_COMPLETED);
        return _run(sched, entity, txn, proto_msg);
    }
}

static
int _module_pack(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn,
                 void* proto_msg)
{
    sk_workflow_t* workflow = sk_txn_workflow(txn);
    sk_module_t* last_module = sk_workflow_last_module(workflow);

    // 1. no pack function means no need to send response
    if (!last_module->pack) {
        sk_txn_setstate(txn, SK_TXN_PACKED);
        return _run(sched, entity, txn, proto_msg);
    }

    // 2. pack the data, and send the response if needed
    last_module->pack(last_module->md, txn);

    size_t packed_data_sz = 0;
    const char* packed_data = sk_txn_output(txn, &packed_data_sz);
    sk_print("module packed data size=%zu\n", packed_data_sz);

    if (!packed_data || !packed_data_sz) {
        sk_print("module no need to send response\n");
    } else {
        sk_print("write data sz:%zu\n", packed_data_sz);
        sk_entity_write(entity, packed_data, packed_data_sz);

        // record metrics
        sk_metrics_worker.response.inc(1);
        sk_metrics_global.response.inc(1);

        unsigned long long alivetime = sk_txn_alivetime(txn);
        sk_print("txn time: %llu\n", alivetime);
        sk_metrics_worker.latency.inc((uint32_t)alivetime);
        sk_metrics_global.latency.inc((uint32_t)alivetime);
    }

    sk_txn_setstate(txn, SK_TXN_PACKED);
    return _run(sched, entity, txn, proto_msg);
}

// Every time pick up the next module in the workflow module list, then execute
// the its `run` method. If reach the last module of the workflow, then will
// execute the `pack` method
static
int _run(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(sched && entity && txn);

    sk_txn_state_t state = sk_txn_state(txn);

    switch (state) {
    case SK_TXN_UNPACKED: {
        sk_print("txn - UNPACKED\n");
        sk_txn_setstate(txn, SK_TXN_RUNNING);
        return _run(sched, entity, txn, proto_msg);
    }
    case SK_TXN_RUNNING: {
        sk_print("txn - RUNNING\n");
        return _module_run(sched, entity, txn, proto_msg);
    }
    case SK_TXN_PENDING: {
        sk_print("txn - PENDING\n");
        sk_txn_setstate(txn, SK_TXN_RUNNING);
        return _run(sched, entity, txn, proto_msg);
    }
    case SK_TXN_COMPLETED: {
        sk_print("txn - COMPLETED\n");
        return _module_pack(sched, entity, txn, proto_msg);
    }
    case SK_TXN_PACKED: {
        sk_print("txn - PACKED: txn destroy\n");
        sk_txn_destroy(txn);
        break;
    }
    default:
        sk_print("Unexpect txn state: %d, ignored\n", state);
        SK_LOG_ERROR(SK_ENV_LOGGER, "Unexpect txn state: %d, ignored", state);
        break;
    }

    return 0;
}

sk_proto_t sk_pto_workflow_run = {
    .priority = SK_PTO_PRI_5,
    .descriptor = &workflow_run__descriptor,
    .run = _run
};
