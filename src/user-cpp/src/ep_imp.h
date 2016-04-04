#ifndef SKULLCPP_EP_IMP_H
#define SKULLCPP_EP_IMP_H

#include "skullcpp/ep.h"

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

} // End of namespace

#endif

