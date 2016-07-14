#ifndef SKULLCPP_EP_IMP_H
#define SKULLCPP_EP_IMP_H

#include "service_imp.h"
#include "skullcpp/ep.h"

namespace skullcpp {

class EPClientImpl {
public:
    std::string        ip_;
    EPClient::UnpackFn unpack_;
    EPClient::Type     type_;
    int                timeout_;
    int                flags_;
    in_port_t          port_;

    uint32_t __padding :16;

public:
    EPClientImpl() : unpack_(NULL), type_(EPClient::TCP),
        timeout_(0), flags_(0), port_(0) {
        (void)__padding;
    }

    ~EPClientImpl() {}
};

class EPClientNPRetImp : public EPClientNPRet {
private:
    EPClient::Status  status_;
    int               latency_;
    const void*       response_;
    size_t            responseSize_;

public:
    EPClientNPRetImp(skull_ep_ret_t ret, const void* response, size_t respSize);
    virtual ~EPClientNPRetImp();

public:
    EPClient::Status status() const;
    int latency() const;
    const void* response() const;
    size_t responseSize() const;
};

class EPClientRetImp : public EPClientRet {
private:
    EPClientNPRetImp  basic_;
    ServiceApiDataImp* apiData_;

public:
    EPClientRetImp(skull_ep_ret_t ret, const void* response,
                   size_t responseSize, ServiceApiDataImp& apiData);
    virtual ~EPClientRetImp();

public:
    EPClient::Status status() const;
    int latency() const;
    const void* response() const;
    size_t responseSize() const;
    ServiceApiData& apiData();
};

} // End of namespace

#endif

