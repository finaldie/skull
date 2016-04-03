#include <stdlib.h>
#include <string.h>

#include <skull/ep.h>

#include "srv_utils.h"
#include "service_imp.h"
#include <skullcpp/ep.h>

namespace skullcpp {

class EPClientImpl {
public:
    std::string       ip_;
    EPClient::unpack  unpack_;
    EPClient::release release_;
    EPClient::Type    type_;
    int               timeout_;
    in_port_t         port_;

#if __WORDSIZE == 64
    uint32_t __padding :16;
    uint32_t __padding1;
#else
    uint32_t __padding :16;
#endif

public:
    EPClientImpl() : unpack_(NULL), release_(NULL), type_(EPClient::TCP),
        timeout_(0), port_(0) {
#if __WORDSIZE == 64
        (void)__padding;
        (void)__padding1;
#else
        (void)__padding;
#endif
    }

    ~EPClientImpl() {}
};

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

void EPClient::setRelease(release releaseFunc) {
    this->impl_->release_ = releaseFunc;
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

    ServiceImp svc(rawSvc);
    EPClient::Ret ret(rawRet);

    const ServiceApiReqRawData* rawData = (const ServiceApiReqRawData*)rawApiReq;
    const std::string& apiName = rawData->apiName;
    ServiceApiReqData apiReq(rawData);
    ServiceApiRespData apiResp(rawSvc, apiName.c_str(), rawApiResp, rawApiRespSz);

    epData->cb(svc, ret, response, len, ud, apiReq.get(), apiResp.get());
}

EPClient::Status EPClient::send(const Service& svc, const void* data, size_t dataSz,
                epCb cb, void* ud) {
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
    epCbData->ud = ud;
    epCbData->release = this->impl_->release_;

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

