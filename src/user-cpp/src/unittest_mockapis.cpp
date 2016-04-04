#include <stdlib.h>
#include "skullcpp/ep.h"
#include "skullcpp/service.h"
#include "service_imp.h"
#include "ep_imp.h"

namespace skullcpp {

/***************************** EPClient Mock APIs *****************************/
EPClient::EPClient() : impl_(new EPClientImpl) {
}

EPClient::~EPClient() {
    delete this->impl_;
}

void EPClient::setType(Type type) {
}

void EPClient::setPort(in_port_t port) {
}

void EPClient::setIP(const std::string& ip) {
}

void EPClient::setTimeout(int timeout) {
}

void EPClient::setUnpack(unpack unpackFunc) {
}

void EPClient::setRelease(release releaseFunc) {
    this->impl_->release_ = releaseFunc;
}

EPClient::Status EPClient::send(const Service& svc, const void* data, size_t dataSz,
                epCb cb, void* ud) {
    if (this->impl_->release_) {
        this->impl_->release_(ud);
    }

    return OK;
}

/***************************** Service Mock APIs ******************************/
int ServiceImp::createJob(Job job, void* ud, uint32_t delayed, int bioIdx) const {
    return 0;
}

} // End of namespace

