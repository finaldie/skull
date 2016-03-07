#include <stdlib.h>
#include "skullcpp/ep.h"

namespace skullcpp {

/***************************** EPClient Mock APIs *****************************/
EPClient::EPClient() : unpack_(NULL), release_(NULL), type_(TCP), timeout_(0),
    port_(0) {}

EPClient::~EPClient() {
}

void EPClient::setType(Type type) {
    this->type_ = type;
}

void EPClient::setPort(in_port_t port) {
    this->port_ = port;
}

void EPClient::setIP(const std::string& ip) {
    this->ip_ = ip;
}

void EPClient::setTimeout(int timeout) {
    this->timeout_ = timeout;
}

void EPClient::setUnpack(unpack unpackFunc) {
    this->unpack_ = unpackFunc;
}

void EPClient::setRelease(release releaseFunc) {
    this->release_ = releaseFunc;
}

EPClient::Status EPClient::send(Service& svc, const void* data, size_t dataSz,
                epCb cb, void* ud) {
    return OK;
}

} // End of namespace

