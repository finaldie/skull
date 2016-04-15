#include <stdlib.h>

#include "ep_imp.h"

namespace skullcpp {

EPClientRetImp::EPClientRetImp(skull_ep_ret_t ret, const void* response,
    size_t responseSize, ServiceApiDataImp& apiData)
    : latency_(ret.latency), response_(response), responseSize_(responseSize),
      apiData_(apiData)
{
    if (ret.status == SKULL_EP_OK) {
        this->status_ = EPClient::Status::OK;
    } else if (ret.status == SKULL_EP_ERROR) {
        this->status_ = EPClient::Status::ERROR;
    } else {
        this->status_ = EPClient::Status::TIMEOUT;
    }
}

EPClientRetImp::~EPClientRetImp() {}

EPClient::Status EPClientRetImp::status() const {
    return this->status_;
}

int EPClientRetImp::latency() const {
    return this->latency_;
}

const void* EPClientRetImp::response() const {
    return this->response_;
}

size_t EPClientRetImp::responseSize() const {
    return this->responseSize_;
}

ServiceApiData& EPClientRetImp::apiData() {
    return this->apiData_;
}

} // End of namespace
