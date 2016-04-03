#include <stdlib.h>
#include "skullcpp/ep.h"
#include "skullcpp/service.h"
#include "service_imp.h"

namespace skullcpp {

/***************************** EPClient Mock APIs *****************************/
EPClient::EPClient() {
}

EPClient::~EPClient() {
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
}

EPClient::Status EPClient::send(const Service& svc, const void* data, size_t dataSz,
                epCb cb, void* ud) {
    return OK;
}

/***************************** Service Mock APIs ******************************/
int ServiceImp::createJob(Job job, void* ud, uint32_t delayed, int bioIdx) {
    return 0;
}

} // End of namespace

