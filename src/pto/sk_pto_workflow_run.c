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

// Every time pick up the next module in the workflow module list, then execute
// the its `run` method. If reach the last module of the workflow, then will
// execute the `pack` method
static
int _run(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    SK_ASSERT(sched && entity && txn);

    // 1. run the next module
    sk_module_t* module = sk_txn_next_module(txn);
    if (!module) {
        SK_LOG_ERROR(SK_ENV_LOGGER, "no module in this workflow, skip it");
        goto module_exit;
    }

    // before run module, set the module name for this module
    // NOTES: the cookie have 256 bytes limitation
    sk_logger_setcookie("module.%s", module->name);

    // run the module
    int ret = module->run(module->md, txn);
    sk_print("module execution return code=%d\n", ret);

    // after module exit, set back the module name
    sk_logger_setcookie(SK_CORE_LOG_COOKIE);

    if (ret) {
        SK_ASSERT_MSG(!ret, "un-implemented code path\n");
        return 1;
    }

    // 2. check whether is the last module:
    // 2.1 if no, send a event for next module
    if (!sk_txn_is_last_module(txn)) {
        sk_print("doesn't reach the last module\n");
        sk_sched_push(sched, entity, txn, SK_PTO_WORKFLOW_RUN, NULL);
        return 0;
    }

    // 2.2 no pack function means no need to send response
    if (!module->pack) {
        goto module_exit;
    }

    // 3. pack the data, and send the response if needed
    module->pack(module->md, txn);

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

module_exit:
    // 4. the transcation is over, destroy sk_txn structure
    sk_print("txn destroy\n");
    sk_txn_destroy(txn);
    return 0;
}

sk_proto_t sk_pto_workflow_run = {
    .priority = SK_PTO_PRI_5,
    .descriptor = &workflow_run__descriptor,
    .run = _run
};
