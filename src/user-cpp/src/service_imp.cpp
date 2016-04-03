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

void ServiceImp::set(const void* data) {
    skull_service_data_set(this->svc, data);
}

void* ServiceImp::get() {
    return skull_service_data(this->svc);
}

const void* ServiceImp::get() const {
    return skull_service_data_const(this->svc);
}

skull_service_t* ServiceImp::getRawService() const {
    return this->svc;
}

/****************************** ServiceApiData ********************************/
ServiceApiDataImp::ServiceApiDataImp() : req_(NULL), resp_(NULL) {}

ServiceApiDataImp::ServiceApiDataImp(ServiceApiReqData* req, ServiceApiRespData* resp)
    : req_(req), resp_(resp) {}

ServiceApiDataImp::~ServiceApiDataImp() {}

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
