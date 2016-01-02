#include <stdlib.h>

#include "flibs/compiler.h"
#include "api/sk_utils.h"
#include "api/sk_workflow.h"
#include "idl_internal.h"
#include "txn_utils.h"

void skull_txn_init(struct _skull_txn_t* skull_txn, const sk_txn_t* txn)
{
    // 1. Restore message
    sk_workflow_t* workflow = sk_txn_workflow(txn);
    const char*    idl_name = workflow->cfg->idl_name;
    const ProtobufCMessageDescriptor* desc = skull_idl_descriptor(idl_name);

    ProtobufCMessage* msg = NULL;
    skull_idl_data_t* idl_data = sk_txn_udata(txn);

    if (likely(idl_data)) {
        msg = protobuf_c_message_unpack(
            desc, NULL, idl_data->data_sz, idl_data->data);
    } else {
        msg = calloc(1, desc->sizeof_message);
        protobuf_c_message_init(desc, msg);
    }

    // 2. fill skull_txn
    skull_txn->txn = (sk_txn_t*) txn;
    skull_txn->idl = msg;
    skull_txn->descriptor = desc;
}

void skull_txn_release(struct _skull_txn_t* skull_txn, sk_txn_t* txn)
{
    ProtobufCMessage* msg      = skull_txn->idl;
    skull_idl_data_t* idl_data = sk_txn_udata(txn);

    size_t new_msg_sz   = protobuf_c_message_get_packed_size(msg);
    void*  new_msg_data = calloc(1, new_msg_sz);
    size_t packed_sz    = protobuf_c_message_pack(msg, new_msg_data);
    SK_ASSERT(new_msg_sz == packed_sz);

    protobuf_c_message_free_unpacked(msg, NULL);
    if (likely(idl_data)) {
        free(idl_data->data);

        idl_data->data    = new_msg_data;
        idl_data->data_sz = packed_sz;
    } else {
        idl_data = calloc(1, sizeof(*idl_data));
        idl_data->data    = new_msg_data;
        idl_data->data_sz = packed_sz;
        sk_txn_setudata(txn, idl_data);
    }
}
