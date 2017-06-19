#ifndef SKULLCPP_CLIENT_H
#define SKULLCPP_CLIENT_H

#include <string>

namespace skullcpp {

class Client {
private:
    // Make noncopyable
    Client(const Client&) = delete;
    Client(Client&&) = delete;

    Client& operator=(const Client&) = delete;
    Client& operator=(Client&&) = delete;

public:
    Client() {}
    virtual ~Client() {}

public:
    typedef enum Type {
        NONE  = 0,
        STD   = 1,
        TCPV4 = 2,
        TCPV6 = 3,
        UDPV4 = 4,
        UDPV6 = 5
    } Type;

public:
    virtual const std::string& name() const = 0;
    virtual int port() const = 0;
    virtual Type type() const = 0;
    virtual const std::string& typeName() const = 0;
};

} // End of namespace

#endif

