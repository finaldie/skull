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

void EPClient::setFlags(int flags) {
    this->impl_->flags_ = flags;
}

void EPClient::setUnpack(UnpackFn unpackFunc) {
    this->impl_->unpack_ = unpackFunc;
}

typedef struct EpCbData {
    EPClientImpl     clientImp_;

    EPClient::EpCb   pendingCb_;
    EPClient::EpNPCb noPendingCb_;
} EpCbData;

static
ssize_t rawUnpackCb(void* ud, const void* data, size_t len) {
    EpCbData* epData = (EpCbData*)ud;

    if (epData->clientImp_.unpack_) {
        return epData->clientImp_.unpack_(data, len);
    } else {
        return (ssize_t)len;
    }
}

static
void rawReleaseCb(void* ud) {
    EpCbData* epData = (EpCbData*)ud;
    delete epData;
}

static
void rawEpPendingCb(const skull_service_t* rawSvc, skull_ep_ret_t rawRet,
             const void* response, size_t len, void* ud,
             const void* rawApiReq, size_t rawApiReqSz,
             void* rawApiResp, size_t rawApiRespSz) {
    EpCbData* epData = (EpCbData*)ud;
    if (!epData->pendingCb_) {
        return;
    }

    skull_service_t* _svc = (skull_service_t*)rawSvc;
    ServiceImp svc(_svc);
    ServiceApiDataImp apiDataImp(rawSvc, rawApiReq, rawApiReqSz, rawApiResp, rawApiRespSz);
    EPClientRetImp ret(epData->clientImp_, rawRet, response, len, apiDataImp);

    epData->pendingCb_(svc, ret);
}

static
void rawEpNoPendingCb(const skull_service_t* rawSvc, skull_ep_ret_t rawRet,
             const void* response, size_t len, void* ud) {
    EpCbData* epData = (EpCbData*)ud;
    if (!epData->noPendingCb_) {
        return;
    }

    ServiceImp svc((skull_service_t*)rawSvc);
    EPClientNPRetImp ret(epData->clientImp_, rawRet, response, len);

    epData->noPendingCb_(svc, ret);
}

EPClient::Status EPClient::send(const Service& svc, const std::string& data,
                                EpCb cb) const {
    return send(svc, data.c_str(), data.length(), cb);
}

EPClient::Status EPClient::send(const Service& svc, const void* data,
                                size_t dataSz, EpCb cb) const {
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

    ep_handler.port    = this->impl_->port_;
    ep_handler.ip      = this->impl_->ip_.c_str();
    ep_handler.timeout = this->impl_->timeout_;
    ep_handler.flags   = this->impl_->flags_;
    ep_handler.unpack  = this->impl_->unpack_ ? rawUnpackCb : NULL;
    ep_handler.release = rawReleaseCb;

    EpCbData* epCbData = new EpCbData;
    epCbData->clientImp_ = *this->impl_;
    epCbData->pendingCb_ = cb;

    skull_ep_status_t st = skull_ep_send(rawSvc, ep_handler, data, dataSz,
                                         rawEpPendingCb, epCbData);
    switch (st) {
    case SKULL_EP_OK:      return OK;
    case SKULL_EP_ERROR:   return ERROR;
    case SKULL_EP_TIMEOUT: return TIMEOUT;
    default: assert(0);    return ERROR;
    }
}

EPClient::Status EPClient::send(const Service& svc,
                                const void* data, size_t dataSz, EpNPCb cb) const
{
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

    ep_handler.port    = this->impl_->port_;
    ep_handler.ip      = this->impl_->ip_.c_str();
    ep_handler.timeout = this->impl_->timeout_;
    ep_handler.flags   = this->impl_->flags_;
    ep_handler.unpack  = this->impl_->unpack_ ? rawUnpackCb : NULL;
    ep_handler.release = rawReleaseCb;

    EpCbData* epCbData = new EpCbData;
    epCbData->clientImp_   = *this->impl_;
    epCbData->noPendingCb_ = cb;

    skull_ep_status_t st = skull_ep_send_np(rawSvc, ep_handler, data, dataSz,
                                            rawEpNoPendingCb, epCbData);
    switch (st) {
    case SKULL_EP_OK:      return OK;
    case SKULL_EP_ERROR:   return ERROR;
    case SKULL_EP_TIMEOUT: return TIMEOUT;
    default: assert(0);    return ERROR;
    }
}

EPClient::Status EPClient::send(const Service& svc, const std::string& data,
                                EpNPCb cb) const
{
    return send(svc, data.c_str(), data.length(), cb);
}

EPClient::Status EPClient::send(const Service& svc,
                                const void* data, size_t dataSz) const {
    return send(svc, data, dataSz, (EpNPCb)NULL);
}

EPClient::Status EPClient::send(const Service& svc, const std::string& data) const {
    return send(svc, data.c_str(), data.length(), (EpNPCb)NULL);
}

} // End of namespace

