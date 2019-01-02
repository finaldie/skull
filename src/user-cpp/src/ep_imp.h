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
    EPClientImpl() : unpack_(nullptr), type_(EPClient::TCP),
        timeout_(0), flags_(0), port_(0) {
        (void)__padding;
    }

    ~EPClientImpl() {}
};

class EPClientNPRetImp : public EPClientNPRet {
private:
    EPClient::Status status_;
    int              latency_;
    const void*      response_;
    size_t           responseSize_;
    EPClientImpl     clientImp_;

public:
    EPClientNPRetImp(const EPClientImpl&, skull_ep_ret_t ret,
                     const void* response, size_t respSize);
    virtual ~EPClientNPRetImp() = default;

public:
    EPClient::Type type() const;
    in_port_t port() const;
    const std::string& ip() const;
    int timeout() const; // unit: millisecond
    int flags() const;

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
    EPClientRetImp(const EPClientImpl&, skull_ep_ret_t ret,
                   const void* response, size_t responseSize,
                   ServiceApiDataImp& apiData);
    virtual ~EPClientRetImp() = default;

public:
    EPClient::Type type() const;
    in_port_t port() const;
    const std::string& ip() const;
    int timeout() const; // unit: millisecond
    int flags() const;

public:
    EPClient::Status status() const;
    int latency() const;
    const void* response() const;
    size_t responseSize() const;

public:
    ServiceApiData& apiData();
    const ServiceApiData& apiData() const;
};

} // End of namespace skullcpp

#endif

