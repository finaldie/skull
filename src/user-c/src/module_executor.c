#include <stdlib.h>
#include <flibs/compiler.h>

#include "api/sk_workflow.h"
#include "api/sk_utils.h"
#include "txn_types.h"
#include "idl_internal.h"

#include "module_executor.h"

void   skull_module_init   (void* md)
{
    sk_c_mdata* mdata = md;
    mdata->init();
}

int    skull_module_run    (void* md, sk_txn_t* txn)
{
    // 1. deserialize the binary data to user layer structure
    sk_workflow_t* workflow = sk_txn_workflow(txn);
    const char* idl_name = workflow->cfg->idl_name;
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

    // 2. run module
    skull_txn_t skull_txn = {
        .txn = txn,
        .idl = msg,
        .descriptor = desc
    };

    sk_c_mdata* mdata = md;
    int ret = mdata->run(&skull_txn);

    // 3. serialize the user layer structure back to the binary data
    size_t new_msg_sz = protobuf_c_message_get_packed_size(msg);
    void *new_msg_data = calloc(1, new_msg_sz);
    size_t packed_sz = protobuf_c_message_pack(msg, new_msg_data);
    SK_ASSERT(new_msg_sz == packed_sz);

    protobuf_c_message_free_unpacked(msg, NULL);
    if (likely(idl_data)) {
        free(idl_data->data);

        idl_data->data = new_msg_data;
        idl_data->data_sz = packed_sz;
    } else {
        idl_data = calloc(1, sizeof(*idl_data));
        idl_data->data = new_msg_data;
        idl_data->data_sz = packed_sz;
        sk_txn_setudata(txn, idl_data);
    }

    return ret;
}

size_t skull_module_unpack (void* md, sk_txn_t* txn,
                            const void* data, size_t data_len)
{
    SK_ASSERT(sk_txn_udata(txn) == NULL);

    // 1. prepare a fresh new user layer idl structure
    sk_workflow_t* workflow = sk_txn_workflow(txn);
    const char* idl_name = workflow->cfg->idl_name;
    const ProtobufCMessageDescriptor* desc = skull_idl_descriptor(idl_name);

    ProtobufCMessage* msg = calloc(1, desc->sizeof_message);
    protobuf_c_message_init(desc, msg);

    // 2. run unpack
    skull_txn_t skull_txn = {
        .txn = txn,
        .idl = msg,
        .descriptor = desc
    };

    sk_c_mdata* mdata = md;
    size_t consumed_sz = mdata->unpack(&skull_txn, data, data_len);

    // 3. serialize the user layer idl data to binary data
    size_t new_msg_sz = protobuf_c_message_get_packed_size(msg);
    void *new_msg_data = calloc(1, new_msg_sz);
    size_t packed_sz = protobuf_c_message_pack(msg, new_msg_data);
    SK_ASSERT(new_msg_sz == packed_sz);
    protobuf_c_message_free_unpacked(msg, NULL);

    skull_idl_data_t* idl_data = calloc(1, sizeof(*idl_data));
    idl_data->data = new_msg_data;
    idl_data->data_sz = packed_sz;
    sk_txn_setudata(txn, idl_data);

    return consumed_sz;
}

void   skull_module_pack   (void* md, sk_txn_t* txn)
{
    // 1. deserialize the binary data to user layer structure
    sk_workflow_t* workflow = sk_txn_workflow(txn);
    const char* idl_name = workflow->cfg->idl_name;
    const ProtobufCMessageDescriptor* desc = skull_idl_descriptor(idl_name);

    skull_idl_data_t* idl_data = sk_txn_udata(txn);

    ProtobufCMessage* msg = protobuf_c_message_unpack(
        desc, NULL, idl_data->data_sz, idl_data->data);

    // 2. run pack
    skull_txn_t skull_txn = {
        .txn = txn,
        .idl = msg,
        .descriptor = desc
    };

    sk_c_mdata* mdata = md;
    mdata->pack(&skull_txn);

    // 3. destroy the user layer structure and the binary data
    protobuf_c_message_free_unpacked(msg, NULL);
    free(idl_data->data);
    free(idl_data);
}
