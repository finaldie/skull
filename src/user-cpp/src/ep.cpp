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

void EPClient::setUnpack(unpack unpackFunc) {
    this->impl_->unpack_ = unpackFunc;
}

typedef struct JobCb {
    EPClient::epPendingCb   pendingCb_;
    EPClient::epNoPendingCb noPendingCb_;
} JobCb;

typedef struct EpCbData {
    EPClient::unpack  unpack_;
    JobCb cb_;
} EpCbData;

static
size_t rawUnpackCb(void* ud, const void* data, size_t len) {
    EpCbData* epData = (EpCbData*)ud;

    if (epData->unpack_) {
        return epData->unpack_(data, len);
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
void rawEpPendingCb(const skull_service_t* rawSvc, skull_ep_ret_t rawRet,
             const void* response, size_t len, void* ud,
             const void* rawApiReq, size_t rawApiReqSz,
             void* rawApiResp, size_t rawApiRespSz) {
    EpCbData* epData = (EpCbData*)ud;
    if (!epData->cb_.pendingCb_) {
        return;
    }

    skull_service_t* _svc = (skull_service_t*)rawSvc;
    ServiceImp svc(_svc);

    const ServiceApiReqRawData* rawData = (const ServiceApiReqRawData*)rawApiReq;
    const std::string& apiName = rawData->apiName;
    ServiceApiReqData  apiReq(rawData);
    ServiceApiRespData apiResp(_svc, apiName.c_str(), rawApiResp, rawApiRespSz);
    ServiceApiDataImp  apiDataImp(&apiReq, &apiResp);
    EPClientRetImp ret(rawRet, response, len, apiDataImp);

    epData->cb_.pendingCb_(svc, ret);
}

static
void rawEpNoPendingCb(const skull_service_t* rawSvc, skull_ep_ret_t rawRet,
             const void* response, size_t len, void* ud) {
    EpCbData* epData = (EpCbData*)ud;
    if (!epData->cb_.noPendingCb_) {
        return;
    }

    ServiceImp svc((skull_service_t*)rawSvc);
    EPClientNPRetImp ret(rawRet, response, len);

    epData->cb_.noPendingCb_(svc, ret);
}

EPClient::Status EPClient::send(const Service& svc, const std::string& data,
                                epPendingCb cb) {
    return send(svc, data.c_str(), data.length(), cb);
}

EPClient::Status EPClient::send(const Service& svc, const void* data,
                                size_t dataSz, epPendingCb cb) {
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
    ep_handler.unpack  = rawUnpackCb;
    ep_handler.release = rawReleaseCb;

    EpCbData* epCbData = new EpCbData;
    epCbData->unpack_  = this->impl_->unpack_;
    epCbData->cb_.pendingCb_ = cb;

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
                                const void* data, size_t dataSz, epNoPendingCb cb)
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
    ep_handler.unpack  = rawUnpackCb;
    ep_handler.release = rawReleaseCb;

    EpCbData* epCbData = new EpCbData;
    epCbData->unpack_   = this->impl_->unpack_;
    epCbData->cb_.noPendingCb_ = cb;

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
                                epNoPendingCb cb)
{
    return send(svc, data.c_str(), data.length(), cb);
}

} // End of namespace

