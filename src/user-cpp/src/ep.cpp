#include <stdlib.h>
#include <string.h>

#include <skull/ep.h>

#include "srv_utils.h"
#include <skullcpp/ep.h>

namespace skullcpp {

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

void EPClient::setIP(std::string& ip) {
    this->ip_ = ip;
}

void EPClient::setTimeout(int timeout) {
    this->timeout_ = timeout;
}

void EPClient::setUnpack(unpack unpackFunc) {
    this->unpack_ = unpackFunc;
}

typedef struct EpCbData {
    EPClient::unpack  unpack;
    EPClient::epCb    cb;
    EPClient::release release;
    void* ud;
} EpCbData;

static
size_t rawUnpackCb(void* ud, const void* data, size_t len) {
    EpCbData* epData = (EpCbData*)ud;

    if (epData->unpack) {
        return epData->unpack(ud, data, len);
    } else {
        return len;
    }
}

static
void rawReleaseCb(void* ud) {
    EpCbData* epData = (EpCbData*)ud;
    if (epData->release) {
        epData->release(epData->ud);
    }

    delete epData;
}

static
void rawEpCb(skull_service_t* rawSvc, skull_ep_ret_t rawRet,
             const void* response, size_t len, void* ud,
             const void* rawApiReq, size_t rawApiReqSz,
             void* rawApiResp, size_t rawApiRespSz) {
    EpCbData* epData = (EpCbData*)ud;
    if (!epData->cb) {
        return;
    }

    Service svc(rawSvc);
    EPClient::Ret ret(rawRet);

    const ServiceApiReqRawData* rawData = (const ServiceApiReqRawData*)rawApiReq;
    const std::string& apiName = rawData->apiName;
    ServiceApiReqData apiReq(rawData);
    ServiceApiRespData apiResp(rawData->svcName.c_str(), apiName.c_str(),
                               rawApiResp, rawApiRespSz);

    epData->cb(svc, ret, response, len, ud, apiReq.get(), apiResp.get());
}

EPClient::Status EPClient::send(Service& svc, const void* data, size_t dataSz,
                epCb cb, void* ud) {
    if (!data || !dataSz) {
        return ERROR;
    }

    skull_service_t* rawSvc = svc.getRawService();
    skull_ep_handler_t ep_handler;
    if (this->type_ == TCP) {
        ep_handler.type = SKULL_EP_TCP;
    } else if (this->type_ == UDP) {
        ep_handler.type = SKULL_EP_UDP;
    } else {
        assert(0);
    }

    ep_handler.port = this->port_;
    ep_handler.ip   = this->ip_.c_str();
    ep_handler.timeout = this->timeout_;
    ep_handler.unpack  = rawUnpackCb;
    ep_handler.release = rawReleaseCb;

    EpCbData* epCbData = new EpCbData;
    epCbData->unpack = this->unpack_;
    epCbData->cb = cb;
    epCbData->ud = ud;
    epCbData->release = this->release_;

    skull_ep_status_t st = skull_ep_send(rawSvc, ep_handler, data, dataSz,
                                         rawEpCb, epCbData);
    switch (st) {
    case SKULL_EP_OK:      return OK;
    case SKULL_EP_ERROR:   return ERROR;
    case SKULL_EP_TIMEOUT: return TIMEOUT;
    default: assert(0); return ERROR;
    }
}

} // End of namespace

