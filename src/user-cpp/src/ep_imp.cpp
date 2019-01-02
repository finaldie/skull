#include "ep_imp.h"

namespace skullcpp {

/****************************** EPClientNPRetImp ******************************/
EPClientNPRetImp::EPClientNPRetImp(const EPClientImpl& clientImp,
                                   skull_ep_ret_t ret, const void* response,
                                   size_t respSize)
    : latency_(ret.latency),
      response_(response),
      responseSize_(respSize),
      clientImp_(clientImp) {
    if (ret.status == SKULL_EP_OK) {
        this->status_ = EPClient::Status::OK;
    } else if (ret.status == SKULL_EP_ERROR) {
        this->status_ = EPClient::Status::ERROR;
    } else {
        this->status_ = EPClient::Status::TIMEOUT;
    }
}

EPClient::Type EPClientNPRetImp::type() const { return clientImp_.type_; }

in_port_t EPClientNPRetImp::port() const { return clientImp_.port_; }

const std::string& EPClientNPRetImp::ip() const { return clientImp_.ip_; }

int EPClientNPRetImp::timeout() const { return clientImp_.timeout_; }

int EPClientNPRetImp::flags() const { return clientImp_.flags_; }

EPClient::Status EPClientNPRetImp::status() const { return this->status_; }

int EPClientNPRetImp::latency() const { return this->latency_; }

const void* EPClientNPRetImp::response() const { return this->response_; }

size_t EPClientNPRetImp::responseSize() const { return this->responseSize_; }

/******************************* EPClientRetImp *******************************/
EPClientRetImp::EPClientRetImp(const EPClientImpl& clientImp,
                               skull_ep_ret_t ret, const void* response,
                               size_t responseSize, ServiceApiDataImp& apiData)
    : basic_(clientImp, ret, response, responseSize), apiData_(&apiData) {}

EPClient::Type EPClientRetImp::type() const { return basic_.type(); }

in_port_t EPClientRetImp::port() const { return basic_.port(); }

const std::string& EPClientRetImp::ip() const { return basic_.ip(); }

int EPClientRetImp::timeout() const { return basic_.timeout(); }

int EPClientRetImp::flags() const { return basic_.flags(); }

EPClient::Status EPClientRetImp::status() const {
    return this->basic_.status();
}

int EPClientRetImp::latency() const { return this->basic_.latency(); }

const void* EPClientRetImp::response() const { return this->basic_.response(); }

size_t EPClientRetImp::responseSize() const {
    return this->basic_.responseSize();
}

ServiceApiData& EPClientRetImp::apiData() { return *this->apiData_; }

const ServiceApiData& EPClientRetImp::apiData() const {
    return *this->apiData_;
}

}  // End of namespace skullcpp
