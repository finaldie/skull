#include <stdlib.h>

#include "ep_imp.h"

namespace skullcpp {

/****************************** EPClientNPRetImp ******************************/
EPClientNPRetImp::EPClientNPRetImp(skull_ep_ret_t ret, const void* response,
    size_t responseSize)
    : latency_(ret.latency), response_(response), responseSize_(responseSize)
{
    if (ret.status == SKULL_EP_OK) {
        this->status_ = EPClient::Status::OK;
    } else if (ret.status == SKULL_EP_ERROR) {
        this->status_ = EPClient::Status::ERROR;
    } else {
        this->status_ = EPClient::Status::TIMEOUT;
    }
}

EPClientNPRetImp::~EPClientNPRetImp() {}

EPClient::Status EPClientNPRetImp::status() const {
    return this->status_;
}

int EPClientNPRetImp::latency() const {
    return this->latency_;
}

const void* EPClientNPRetImp::response() const {
    return this->response_;
}

size_t EPClientNPRetImp::responseSize() const {
    return this->responseSize_;
}

/******************************* EPClientRetImp *******************************/
EPClientRetImp::EPClientRetImp(skull_ep_ret_t ret, const void* response,
                   size_t responseSize, ServiceApiDataImp& apiData)
    : basic_(ret, response, responseSize), apiData_(&apiData)
{
}

EPClientRetImp::~EPClientRetImp() {}

EPClient::Status EPClientRetImp::status() const {
    return this->basic_.status();
}

int EPClientRetImp::latency() const {
    return this->basic_.latency();
}

const void* EPClientRetImp::response() const {
    return this->basic_.response();
}

size_t EPClientRetImp::responseSize() const {
    return this->basic_.responseSize();
}

ServiceApiData& EPClientRetImp::apiData() {
    return *this->apiData_;
}

} // End of namespace

