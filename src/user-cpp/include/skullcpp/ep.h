#ifndef SKULLCPP_EP_H
#define SKULLCPP_EP_H

#include <stddef.h>
#include <netinet/in.h>

#include <string>
#include <memory>
#include <functional>

#include <skull/service.h>
#include <skull/ep.h>
#include <google/protobuf/message.h>

#include <skullcpp/service.h>

namespace skullcpp {

class EPClientImpl;
class EPClientRet;

class EPClient {
private:
    // Make noncopyable
    EPClient(const EPClient&);
    const EPClient& operator=(const EPClient&);

private:
    EPClientImpl* impl_;

public:
    typedef enum Type {
        TCP = 0,
        UDP = 1
    } Type;

    typedef enum Status {
        OK      = 0,
        ERROR   = 1,
        TIMEOUT = 2
    } Status;

    // Case 1: Be called from a service api call, then user can get api req and
    //          resp from 'apiData'
    // Case 2: Be called from a service job, then the 'ret.apiData' is useless,
    //          and DO NOT try to use it
    typedef std::function<void (const Service&, EPClientRet& ret)> epCb;

    typedef std::function<size_t (const void* data, size_t len)> unpack;

// Use this macro to make a unpack/epCb easily
#define skull_BindEp(f, ...) \
    std::bind(f, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)

public:
    EPClient();
    ~EPClient();

public:
    void setType(Type type);
    void setPort(in_port_t port);
    void setIP(const std::string& ip);

    // unit: millisecond
    // <= 0: means no timeout
    // >  0: after x milliseconds, the ep call would time out
    void setTimeout(int timeout);
    void setUnpack(unpack unpackFunc);

public:
    Status send(const Service&, const void* data, size_t dataSz, epCb cb);
    Status send(const Service&, const std::string& data, epCb cb);
};

class EPClientRet {
private:
    // Make noncopyable
    EPClientRet(const EPClientRet&);
    const EPClientRet& operator=(const EPClientRet&);

public:
    EPClientRet() {}
    virtual ~EPClientRet() {}

public:
    virtual EPClient::Status status() const = 0;
    virtual int latency() const = 0;
    virtual const void* response() const = 0;
    virtual size_t responseSize() const = 0;
    virtual ServiceApiData& apiData() = 0;
};

} // End of namespace

#endif

