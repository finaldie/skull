#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "skull/service.h"
#include "skull/txn.h"

#include "srv_loader.h"
#include "srv_utils.h"
#include "service_imp.h"
#include "skullcpp/service.h"

namespace skullcpp {

ServiceImp::ServiceImp(skull_service_t* svc) {
    this->svc = svc;
}

ServiceImp::~ServiceImp() {
}

void ServiceImp::set(ServiceData* data) {
    skull_service_data_set(this->svc, data);
}

ServiceData* ServiceImp::get() {
    return (ServiceData*)skull_service_data(this->svc);
}

const ServiceData* ServiceImp::get() const {
    return (const ServiceData*)skull_service_data_const(this->svc);
}

skull_service_t* ServiceImp::getRawService() const {
    return this->svc;
}

/****************************** ServiceApiData ********************************/
ServiceApiDataImp::ServiceApiDataImp() : req_(NULL), resp_(NULL) {}

ServiceApiDataImp::ServiceApiDataImp(ServiceApiReqData* req, ServiceApiRespData* resp)
    : req_(req), resp_(resp), cleanup(false) {}

ServiceApiDataImp::ServiceApiDataImp(skull_service_t* svc,
    const void* apiReq, size_t apiReqSz, void* apiResp, size_t apiRespSz)
    : req_(NULL), resp_(NULL), cleanup(true) {
    // Construct api request
    const ServiceApiReqRawData* rawData = (const ServiceApiReqRawData*)apiReq;
    this->req_ = new ServiceApiReqData(rawData);

    // Construct api response
    const std::string& apiName = rawData->apiName;
    this->resp_ = new ServiceApiRespData(svc, apiName.c_str(), apiResp, apiRespSz);
}

ServiceApiDataImp::ServiceApiDataImp(const skull_service_t* svc,
    const void* apiReq, size_t apiReqSz, void* apiResp, size_t apiRespSz)
    : req_(NULL), resp_(NULL), cleanup(true) {
    // Construct api request
    const ServiceApiReqRawData* rawData = (const ServiceApiReqRawData*)apiReq;
    this->req_ = new ServiceApiReqData(rawData);

    // Construct api response
    const std::string& apiName = rawData->apiName;
    this->resp_ = new ServiceApiRespData(
        (skull_service_t*)svc, apiName.c_str(), apiResp, apiRespSz);
}

ServiceApiDataImp::~ServiceApiDataImp() {
    if (!cleanup) return;

    delete this->req_;
    delete this->resp_;
}

bool ServiceApiDataImp::valid() const {
    return req_ && resp_;
}

const google::protobuf::Message& ServiceApiDataImp::request() const {
    if (!this->valid()) {
        assert(0);
    }

    return this->req_->get();
}

google::protobuf::Message& ServiceApiDataImp::response() const {
    if (!this->valid()) {
        assert(0);
    }

    return this->resp_->get();
}

} // End of namespace
