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
void _execute_module(sk_sched_t* sched, sk_txn_t* txn,
                     const char* data, size_t sz)
{
    sk_entity_t* entity = sk_txn_entity(txn);
    sk_txn_status_t status = sk_txn_status(txn);

    sk_module_t* module = NULL;
    if (status == SK_TXN_INIT) {
        module = sk_txn_current_module(txn);
    } else if (status == SK_TXN_START) {
        module = sk_txn_next_module(txn);
    }

    int ret = module->sk_module_run(txn);
    sk_print("module execution return code=%d\n", ret);

    if (ret) {
        sk_txn_set_status(txn, SK_TXN_ERROR);
        SK_ASSERT_MSG(!ret, "un-implemented code path\n");
        return;
    }

    if (status == SK_TXN_INIT) {
        sk_txn_set_status(txn, SK_TXN_START);
    } else if (status == SK_TXN_START) {
        if (!sk_txn_is_last_module(txn)) {
            return;
        }

        sk_txn_set_status(txn, SK_TXN_END);

        // pack the data, and send the response if needed
        module->sk_module_pack(txn);

        size_t data_sz = 0;
        const char* data = sk_txn_output(txn, &data_sz);
        sk_print("module packed data size=%zu\n", data_sz);

        if (!data) {
            sk_print("module no need to send response\n");
            return;
        }

        sk_entity_write(entity, data, sz);

    } else if (status == SK_TXN_END) {
        sk_txn_destroy(txn);
        sk_entity_dec_task_cnt(entity);
    }
}

// consume the data received from network
static
int _run(sk_sched_t* sched, sk_txn_t* txn, void* proto_msg)
{
    NetProc* net_msg = proto_msg;
    size_t sz = net_msg->data.len;
    unsigned char* data = net_msg->data.data;

    _execute_module(sched, txn, (const char*)data, sz);

    //char* tmp = calloc(1, sz + 1);
    //memcpy(tmp, data, sz);
    //sk_print("receive data: [%s], sz=%zu\n", tmp, sz);
    //sk_print("echo back the data [%s]\n", tmp);
    //sk_entity_write(entity, data, sz);
    //free(tmp);

    return 0;
}

sk_proto_t sk_pto_net_proc = {
    .priority = SK_PTO_PRI_5,
    .descriptor = &net_proc__descriptor,
    .run = _run
};
