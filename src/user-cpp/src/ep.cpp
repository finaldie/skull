#include <stdlib.h>
#include <string.h>

#include <skull/ep.h>

#include "srv_utils.h"
#include "service_imp.h"
#include "ep_imp.h"
#include <skullcpp/ep.h>

namespace skullcpp {

EPClient::EPClient() : impl_(new EPClientImpl) {
}

EPClient::~EPClient() {
    delete this->impl_;
}

void EPClient::setType(Type type) {
    this->impl_->type_ = type;
}

void EPClient::setPort(in_port_t port) {
    this->impl_->port_ = port;
}

void EPClient::setIP(const std::string& ip) {
    this->impl_->ip_ = ip;
}

void EPClient::setTimeout(int timeout) {
    this->impl_->timeout_ = timeout;
}

void EPClient::setUnpack(unpack unpackFunc) {
    this->impl_->unpack_ = unpackFunc;
}

typedef struct EpCbData {
    EPClient::unpack  unpack;
    EPClient::epCb    cb;
} EpCbData;

static
size_t rawUnpackCb(void* ud, const void* data, size_t len) {
    EpCbData* epData = (EpCbData*)ud;

    if (epData->unpack) {
        return epData->unpack(data, len);
    } else {
        return len;
    }
}

static
void rawReleaseCb(void* ud) {
    EpCbData* epData = (EpCbData*)ud;
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

    ServiceImp svc(rawSvc);

    // Be called from service job
    if (!rawApiReqSz && !rawApiRespSz) {
        ServiceApiDataImp apiDataImp;
        EPClientRetImp ret(rawRet, response, len, apiDataImp);

        epData->cb(svc, ret);
    } else { // Be called from a normal service api
        const ServiceApiReqRawData* rawData = (const ServiceApiReqRawData*)rawApiReq;
        const std::string& apiName = rawData->apiName;
        ServiceApiReqData apiReq(rawData);
        ServiceApiRespData apiResp(rawSvc, apiName.c_str(), rawApiResp, rawApiRespSz);
        ServiceApiDataImp apiDataImp(&apiReq, &apiResp);
        EPClientRetImp ret(rawRet, response, len, apiDataImp);

        epData->cb(svc, ret);
    }
}

EPClient::Status EPClient::send(const Service& svc, const void* data,
                                size_t dataSz, epCb cb) {
    if (!data || !dataSz) {
        return ERROR;
    }

    const ServiceImp& svcImp = (const ServiceImp&)svc;
    skull_service_t* rawSvc = svcImp.getRawService();
    skull_ep_handler_t ep_handler;
    memset(&ep_handler, 0, sizeof(ep_handler));

    if (this->impl_->type_ == TCP) {
        ep_handler.type = SKULL_EP_TCP;
    } else if (this->impl_->type_ == UDP) {
        ep_handler.type = SKULL_EP_UDP;
    } else {
        assert(0);
    }

    ep_handler.port = this->impl_->port_;
    ep_handler.ip   = this->impl_->ip_.c_str();
    ep_handler.timeout = this->impl_->timeout_;
    ep_handler.unpack  = rawUnpackCb;
    ep_handler.release = rawReleaseCb;

    EpCbData* epCbData = new EpCbData;
    epCbData->unpack = this->impl_->unpack_;
    epCbData->cb = cb;

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

