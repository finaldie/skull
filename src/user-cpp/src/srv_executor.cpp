#include <stdlib.h>
#include <string.h>

#include "skullcpp/service.h"
#include "srv_loader.h"
#include "srv_utils.h"

#include "srv_executor.h"

using namespace skullcpp;

void skull_srv_init (skull_service_t* srv, void* data)
{
    srvdata_t* srv_data = (srvdata_t*)data;
    ServiceEntry* entry = srv_data->entry;

    Service svc(srv);

    entry->init(svc, srv_data->config);
}

void skull_srv_release (skull_service_t* srv, void* data)
{
    srvdata_t* srv_data = (srvdata_t*)data;
    ServiceEntry* entry = srv_data->entry;

    Service svc(srv);

    entry->release(svc);
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

    ServiceReadApi* rApi = FindApi<ServiceReadApi>(entry->rApis, api_name);
    if (rApi) {
        Service svc(srv);
        ServiceApiReqData  apiReq(srv, api_name);
        ServiceApiRespData apiResp(srv, api_name, true);

        rApi->iocall(svc, apiReq.get(), apiResp.get());
        return 0;
    }

    ServiceWriteApi* wApi = FindApi<ServiceWriteApi>(entry->wApis, api_name);
    if (wApi) {
        Service svc(srv);
        ServiceApiReqData  apiReq(srv, api_name);
        ServiceApiRespData apiResp(srv, api_name, true);

        rApi->iocall(svc, apiReq.get(), apiResp.get());
        return 0;
    }

    // Cannot find any read/write api to execute
    return 1;
}

void skull_srv_iocomplete(skull_service_t* srv, const char* api_name, void* data)
{
    // 1. Release api request data
    ServiceApiReqRawData* apiReq =
        (ServiceApiReqRawData*)skull_service_apidata(srv, SKULL_API_REQ, NULL);
    delete apiReq;

    // 2. Release api response data
    void* apiResp = skull_service_apidata(srv, SKULL_API_RESP, NULL);
    free(apiResp);
}

