#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fev/fev_buff.h"
#include "api/sk_utils.h"
#include "api/sk_event.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_pto.h"
#include "api/sk_txn.h"
#include "api/sk_sched.h"

static
int _execute_module(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn,
                     const char* data, size_t sz)
{
    sk_module_t* module = sk_txn_current_module(txn);

    int ret = module->sk_module_run(txn);
    sk_print("module execution return code=%d\n", ret);

    if (ret) {
        SK_ASSERT_MSG(!ret, "un-implemented code path\n");
        return 1;
    }

    if (!sk_txn_is_last_module(txn)) {
        sk_print("doesn't reach the last module\n");
        sk_txn_next_module(txn);
        sk_sched_push(sched, entity, txn, SK_PTO_NET_PROC, NULL);
        return 0;
    }

    // pack the data, and send the response if needed
    module->sk_module_pack(txn);

    size_t unpacked_data_sz = 0;
    const char* unpacked_data = sk_txn_output(txn, &unpacked_data_sz);
    sk_print("module packed data size=%zu\n", unpacked_data_sz);

    if (!unpacked_data) {
        sk_print("module no need to send response\n");
    } else {
        sk_print("write data sz:%zu\n", unpacked_data_sz);
        sk_entity_write(entity, unpacked_data, unpacked_data_sz);
    }

    sk_print("txn destroy\n");
    sk_txn_destroy(txn);
    sk_entity_dec_task_cnt(entity);
    return 0;
}

// consume the data received from network
static
int _run(sk_sched_t* sched, sk_entity_t* entity, sk_txn_t* txn, void* proto_msg)
{
    NetProc* net_msg = proto_msg;
    size_t sz = net_msg->data.len;
    unsigned char* data = net_msg->data.data;

    return _execute_module(sched, entity, txn, (const char*)data, sz);
}

sk_proto_t sk_pto_net_proc = {
    .priority = SK_PTO_PRI_5,
    .descriptor = &net_proc__descriptor,
    .run = _run
};
