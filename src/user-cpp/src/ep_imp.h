#ifndef SKULLCPP_EP_IMP_H
#define SKULLCPP_EP_IMP_H

#include "service_imp.h"
#include "skullcpp/ep.h"

namespace skullcpp {

class EPClientImpl {
public:
    std::string       ip_;
    EPClient::unpack  unpack_;
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
    EPClientImpl() : unpack_(NULL), type_(EPClient::TCP),
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

class EPClientRetImp : public EPClientRet {
private:
    EPClient::Status status_;
    int latency_;
    const void* response_;
    size_t responseSize_;
    ServiceApiDataImp apiData_;

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

