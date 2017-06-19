#ifndef SKULLCPP_CLIENT_IMP_H
#define SKULLCPP_CLIENT_IMP_H

#include "skullcpp/client.h"

namespace skullcpp {

class ClientImp : public Client {
private:
    std::string name_;
    Type        type_;
    int         port_;

public:
    ClientImp(Type type, const std::string& name, int port)
        : name_(name), type_(type), port_(port) {}

    virtual ~ClientImp() {}

public:
    const std::string& name() const;
    int port() const;
    Type type() const;
    const std::string& typeName() const;
};

} // End of namespace

#endif

