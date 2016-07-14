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

void EPClient::setUnpack(UnpackFn unpackFunc) {
}

EPClient::Status EPClient::send(const Service& svc, const void* data,
                                size_t dataSz, EpCb cb) {
    return OK;
}

EPClient::Status EPClient::send(const Service& svc, const std::string& data,
                                EpCb cb) {
    return OK;
}

EPClient::Status EPClient::send(const Service& svc, const void* data,
                                size_t dataSz, EpNPCb cb) {
    return OK;
}

EPClient::Status EPClient::send(const Service& svc, const std::string& data,
                                EpNPCb cb) {
    return OK;
}

/***************************** Service Mock APIs ******************************/
int ServiceImp::createJob(uint32_t delayed, Job job) const {
    return 0;
}

int ServiceImp::createJob(Job job) const {
    return 0;
}

int ServiceImp::createJob(uint32_t delayed, int bioIdx, JobNP job) const {
    return 0;
}

int ServiceImp::createJob(uint32_t delayed, JobNP job) const {
    return 0;
}

int ServiceImp::createJob(int bioIdx, JobNP job) const {
    return 0;
}

int ServiceImp::createJob(JobNP job) const {
    return 0;
}

} // End of namespace

