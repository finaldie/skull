#include <stdlib.h>
#include <string.h>

#include "api/sk_utils.h"
#include "idl_internal.h"
#include "txn_utils.h"
#include "srv_types.h"
#include "srv_loader.h"
#include "srv_utils.h"
#include "srv_executor.h"

void skull_srv_init (sk_service_t* srv, void* data)
{
    skull_c_srvdata_t* srv_data = data;
    skull_service_entry_t* entry = srv_data->entry;

    skull_service_t skull_service = {
        .service = srv
    };

    entry->init(&skull_service, srv_data->config);
}

void skull_srv_release (sk_service_t* srv, void* data)
{
    skull_c_srvdata_t* srv_data = data;
    skull_service_entry_t* entry = srv_data->entry;

    skull_service_t skull_service = {
        .service = srv
    };

    entry->release(&skull_service);
}

/**
 * Invoke the real user service api function
 */
int  skull_srv_iocall  (sk_service_t* srv, sk_txn_t* txn, void* sdata,
                        uint64_t task_id, const char* api_name,
                        sk_srv_io_status_t ustatus)
{
    // find the api func
    skull_c_srvdata_t* srv_data = sdata;
    skull_service_entry_t* entry = srv_data->entry;

    skull_service_t skull_service = {
        .service = srv
    };

    skull_service_async_api_t* api = skull_svc_find_api(entry->async, api_name);
    if (!api) {
        return 1;
    }

    // get the task data
    sk_txn_taskdata_t* task_data = sk_txn_taskdata(txn, task_id);
    SK_ASSERT(task_data);

    // construct empty response
    char resp_proto_name[SKULL_SRV_PROTO_MAXLEN];
    snprintf(resp_proto_name, SKULL_SRV_PROTO_MAXLEN, "%s.%s_resp",
             sk_service_name(srv), api_name);

    const ProtobufCMessageDescriptor* resp_desc =
        skull_srv_idl_descriptor(resp_proto_name);
    SK_ASSERT(resp_desc);

    ProtobufCMessage* resp_msg = NULL;
    resp_msg = calloc(1, resp_desc->sizeof_message);
    protobuf_c_message_init(resp_desc, resp_msg);

    // restore the request
    char req_proto_name[SKULL_SRV_PROTO_MAXLEN];
    snprintf(req_proto_name, SKULL_SRV_PROTO_MAXLEN, "%s.%s_req",
             sk_service_name(srv), api_name);

    const ProtobufCMessageDescriptor* req_desc =
        skull_srv_idl_descriptor(req_proto_name);

    ProtobufCMessage* req_msg = NULL;
    req_msg = protobuf_c_message_unpack(
            req_desc, NULL, task_data->request_sz, task_data->request);

    // invoke the user service api
    api->iocall(&skull_service, req_msg, resp_msg);

    // fill api callback response pb message
    if (task_data->cb) {
        task_data->response_pb_msg = resp_msg;
    }

    // clean up the req and resp
    protobuf_c_message_free_unpacked(req_msg, NULL);
    if (!task_data->cb) {
        protobuf_c_message_free_unpacked(resp_msg, NULL);
    }
    return 0;
}

int skull_srv_iocall_complete(sk_service_t* srv, sk_txn_t* txn, void* sdata,
                                uint64_t task_id, const char* api_name)
{
    // get the task data
    sk_txn_taskdata_t* task_data = sk_txn_taskdata(txn, task_id);
    SK_ASSERT(task_data);

    if (!task_data->cb) {
        sk_print("no service api callback function to run, skip it\n");
        return 0;
    }

    // restore the request
    char req_proto_name[SKULL_SRV_PROTO_MAXLEN];
    snprintf(req_proto_name, SKULL_SRV_PROTO_MAXLEN, "%s.%s_req",
             sk_service_name(srv), api_name);

    const ProtobufCMessageDescriptor* req_desc =
        skull_srv_idl_descriptor(req_proto_name);

    ProtobufCMessage* req_msg = NULL;
    req_msg = protobuf_c_message_unpack(
            req_desc, NULL, task_data->request_sz, task_data->request);

    // Run api callback
    skull_txn_t skull_txn;
    skull_txn_init(&skull_txn, txn);

    int ret = ((skull_svc_api_cb)task_data->cb)(&skull_txn, req_msg,
                                               task_data->response_pb_msg);

    // Release unpacked resources
    skull_txn_release(&skull_txn, txn);

    protobuf_c_message_free_unpacked(req_msg, NULL);
    protobuf_c_message_free_unpacked(task_data->response_pb_msg, NULL);
    task_data->response_pb_msg = NULL;
    return ret;
}
