#include <stdlib.h>
#include <string.h>

#include "skull/service.h"
#include "skull/txn.h"

#include "srv_loader.h"
#include "srv_utils.h"
#include "skullcpp/service.h"

namespace skullcpp {

Service::Service(skull_service_t* svc) {
    this->svc = svc;
}

Service::~Service() {
}

void Service::set(const void* data) {
    skull_service_data_set(this->svc, data);
}

void* Service::get() {
    return skull_service_data(this->svc);
}

const void* Service::get() const {
    return skull_service_data_const(this->svc);
}

static
int _skull_svc_api_callback(skull_txn_t* sk_txn, const char* apiName,
                            const void* request, size_t req_sz,
                            const void* response, size_t resp_sz) {
    Txn txn(sk_txn);
    const ServiceApiReqRawData* rawData = (const ServiceApiReqRawData*)request;
    ServiceApiReqData apiReq(rawData);
    ServiceApiRespData apiResp(rawData->svcName.c_str(), apiName, response, resp_sz);

    return rawData->cb(txn, std::string(apiName), apiReq.get(), apiResp.get());
}

Service::Status ServiceCall (Txn& txn,
                             const char* serviceName,
                             const char* apiName,
                             const google::protobuf::Message& request,
                             Service::ApiCB cb,
                             int bioIdx) {
    // 1. Construct raw req data
    ServiceApiReqData apiReq(serviceName, apiName, request, cb);
    size_t dataSz = 0;
    ServiceApiReqRawData* rawReqData = apiReq.serialize(dataSz);

    // 2. Send service call to core
    skull_service_ret_t ret =
        skull_service_async_call(txn.txn(), serviceName, apiName, rawReqData,
            dataSz, _skull_svc_api_callback, bioIdx);

    // 3. Return status code
    switch (ret) {
    case SKULL_SERVICE_OK:
        return Service::OK;
    case SKULL_SERVICE_ERROR_SRVNAME:
        return Service::ERROR_SRVNAME;
    case SKULL_SERVICE_ERROR_APINAME:
        return Service::ERROR_APINAME;
    case SKULL_SERVICE_ERROR_BIO:
        return Service::ERROR_BIO;
    default:
        assert(0);
        return Service::OK;
    }
}

skull_service_t* Service::getRawService() const {
    return this->svc;
}

} // End of namespace
