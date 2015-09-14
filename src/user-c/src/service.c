#include <stdlib.h>
#include <string.h>

#include "api/sk_core.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_utils.h"

#include "skull/txn.h"
#include "txn_types.h"
#include "srv_types.h"
#include "srv_loader.h"
#include "skull/service.h"

static
skull_service_async_api_t* _find_api(skull_service_async_api_t** apis,
                                     const char* api_name)
{
    if (!apis) {
        return NULL;
    }

    for (int i = 0; apis[i] != NULL; i++) {
        skull_service_async_api_t* api = apis[i];

        if (0 == strcmp(api->name, api_name)) {
            return api;
        }
    }

    return NULL;
}

skull_service_ret_t
skull_service_async_call (skull_txn_t* txn, const char* service_name,
                          const char* api_name, const void* request,
                          skull_module_cb cb)
{
    sk_core_t* core = SK_ENV_CORE;
    sk_service_t* service = sk_core_get_service(core, service_name);
    if (!service) {
        return SKULL_SERVICE_ERROR_SRVNAME;
    }

    // find the exact async api
    sk_service_opt_t* opt = sk_service_opt(service);
    skull_c_srvdata* srv_data = opt->srv_data;
    skull_service_entry_t* entry = srv_data->entry;
    skull_service_async_api_t** async_apis = entry->async;
    skull_service_async_api_t* api = _find_api(async_apis, api_name);

    if (!api) {
        return SKULL_SERVICE_ERROR_APINAME;
    }

    // serialize the request data
    size_t req_sz = protobuf_c_message_get_packed_size(request);
    void* serialized_req = calloc(1, req_sz);
    size_t packed_sz = protobuf_c_message_pack(request, serialized_req);
    SK_ASSERT(req_sz == packed_sz);

    sk_service_iocall(service, txn->txn, api_name,
                      serialized_req, req_sz,
                      (sk_txn_module_cb)cb, txn);

    return SKULL_SERVICE_OK;
}

void  skull_service_data_set (skull_service_t* service, const void* data)
{
    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);

    sk_service_data_set(sk_srv, data);
}

void* skull_service_data (skull_service_t* service)
{
    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);
    return sk_service_data(sk_srv);
}

const void* skull_service_data_const (skull_service_t* service)
{
    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);
    return sk_service_data_const(sk_srv);
}
