#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fev/fev_buff.h"
#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_event.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_pto.h"
#include "api/sk_txn.h"
#include "api/sk_log.h"
#include "api/sk_env.h"
#include "api/sk_sched.h"

#define SK_MAX_COOKIE_LEN 256

// Every time pick up the next module in the workflow module list, then execute
// the its `run` method. If reach the last module of the workflow, then will
// execute the `pack` method
static
int _run(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    // 0. First, if the current module is the first module (deserialization
    // module), so we will increase the request counter metrcis
    if (sk_txn_is_first_module(txn)) {
        SK_THREAD_ENV->monitor->request.inc(1);
        SK_THREAD_ENV_CORE->monitor->request.inc(1);
    }

    // 1. run the next module
    sk_module_t* module = sk_txn_next_module(txn);

    // before run module, set the module name for this module
    // NOTES: the cookie have 256 bytes limitation
    sk_logger_setcookie("module.%s", module->name);

    // run the module
    int ret = module->sk_module_run(txn);
    sk_print("module execution return code=%d\n", ret);

    // after module exit, set back the module name
    sk_logger_setcookie(SK_CORE_LOG_COOKIE);

    if (ret) {
        SK_ASSERT_MSG(!ret, "un-implemented code path\n");
        return 1;
    }

    // 2. check whether is the last module:
    // if no, send a event for next module
    // if yes, pack
    if (!sk_txn_is_last_module(txn)) {
        sk_print("doesn't reach the last module\n");
        sk_sched_push(sched, entity, txn, SK_PTO_NET_PROC, NULL);
        return 0;
    }

    // 3. pack the data, and send the response if needed
    module->sk_module_pack(txn);

    size_t packed_data_sz = 0;
    const char* packed_data = sk_txn_output(txn, &packed_data_sz);
    sk_print("module packed data size=%zu\n", packed_data_sz);

    if (!packed_data) {
        sk_print("module no need to send response\n");
    } else {
        sk_print("write data sz:%zu\n", packed_data_sz);
        sk_entity_write(entity, packed_data, packed_data_sz);

        // record metrics
        SK_THREAD_ENV->monitor->response.inc(1);
        SK_THREAD_ENV_CORE->monitor->response.inc(1);

        unsigned long long alivetime = sk_txn_alivetime(txn);
        SK_THREAD_ENV->monitor->latency.inc((uint32_t)(alivetime / 1000));
        SK_THREAD_ENV_CORE->monitor->latency.inc((uint32_t)(alivetime / 1000));
    }

    // 4. the transcation is over, destroy sk_txn structure
    sk_print("txn destroy\n");
    sk_txn_destroy(txn);
    sk_entity_dec_task_cnt(entity);
    return 0;
}

sk_proto_t sk_pto_net_proc = {
    .priority = SK_PTO_PRI_5,
    .descriptor = &net_proc__descriptor,
    .run = _run
};
