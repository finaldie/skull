#include <stdlib.h>
#include <string.h>

#include "skullcpp/service.h"
#include "skullcpp/logger.h"

#include "srv_loader.h"
#include "srv_utils.h"
#include "service_imp.h"

#include "srv_executor.h"

namespace skullcpp {

template<class T>
void _register_svc_api(skull_service_t* srv, T* apis, skull_service_api_type_t type) {
    if (!apis) return;

    for (int i = 0; apis[i].name != NULL; i++) {
        T* api = &apis[i];

        skull_service_api_register(srv, api->name, type);
    }
}

int  skull_srv_init (skull_service_t* srv, void* data)
{
    srvdata_t* srv_data = (srvdata_t*)data;
    ServiceEntry* entry = srv_data->entry;

    // 1. Register service apis
    _register_svc_api(srv, entry->rApis, SKULL_SVC_API_READ);
    _register_svc_api(srv, entry->wApis, SKULL_SVC_API_WRITE);

    // 2. Execute user-init
    ServiceImp svc(srv);

    try {
        return entry->init(svc, srv_data->config);
    } catch (std::exception& e) {
        SKULL_LOG_ERROR("service.init", "Exception: %s", e.what());
    }

    // Error occurred
    return -1;
}

void skull_srv_release (skull_service_t* srv, void* data)
{
    srvdata_t* srv_data = (srvdata_t*)data;
    ServiceEntry* entry = srv_data->entry;

    ServiceImp svc(srv);

    try {
        entry->release(svc);
    } catch (std::exception& e) {
        SKULL_LOG_ERROR("service.release", "Exception: %s", e.what());
    }

    auto* svcData = (ServiceData*)skull_service_data(srv);
    if (svcData) {
        delete svcData;
        skull_service_data_set(srv, NULL);
    }
}

/**
 * Invoke the real user service api function
 * Notes: This api can be ran on worker/bio
 */
int  skull_srv_iocall  (skull_service_t* srv, const char* api_name,
                        void* data)
{
    // find the api func
    srvdata_t* srv_data = (srvdata_t*)data;
    ServiceEntry* entry = srv_data->entry;

    try {
        ServiceReadApi* rApi = FindApi<ServiceReadApi>(entry->rApis, api_name);
        if (rApi) {
            ServiceImp svc(srv);
            ServiceApiReqData  apiReq(srv, api_name);
            ServiceApiRespData apiResp(srv, api_name, true);

            rApi->iocall(svc, apiReq.get(), apiResp.get());
            return 0;
        }

        ServiceWriteApi* wApi = FindApi<ServiceWriteApi>(entry->wApis, api_name);
        if (wApi) {
            ServiceImp svc(srv);
            ServiceApiReqData  apiReq(srv, api_name);
            ServiceApiRespData apiResp(srv, api_name, true);

            wApi->iocall(svc, apiReq.get(), apiResp.get());
            return 0;
        }
    } catch (std::exception& e) {
        SKULL_LOG_ERROR("service.iocall", "Exception: %s", e.what());
    }

    // Cannot find any read/write api to execute
    return 1;
}

void skull_srv_iocomplete(skull_service_t* srv, const char* api_name, void* data)
{
    // 1. Reset api request data to NULL. The caller is responsible for releasing
    //     the data
    skull_service_apidata_set(srv, SKULL_API_REQ, NULL, 0);

    // 2. Release api response data
    void* apiResp = skull_service_apidata(srv, SKULL_API_RESP, NULL);
    free(apiResp);

    skull_service_apidata_set(srv, SKULL_API_RESP, NULL, 0);
}

} // End of namespace

