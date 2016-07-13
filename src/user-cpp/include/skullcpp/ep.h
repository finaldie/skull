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
class EPClientNPRet;

class EPClient {
private:
    // Make noncopyable
    EPClient(const EPClient&) = delete;
    EPClient(EPClient&&) = delete;
    EPClient& operator=(const EPClient&) = delete;
    EPClient& operator=(EPClient&&) = delete;

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

    typedef std::function<void (const Service&, EPClientRet& ret)>   epPendingCb;
    typedef std::function<void (const Service&, EPClientNPRet& ret)> epNoPendingCb;

    typedef std::function<size_t (const void* data, size_t len)> unpack;

// Use this macro to make a unpack/epCb easily
#define skull_BindEp(f, ...) \
    std::bind(f, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)

// Flags
#define SKULLCPP_EP_F_CONCURRENT 0x1
#define SKULLCPP_EP_F_ORPHAN     0x2

public:
    EPClient();
    ~EPClient();

public:
    void setType(Type type);
    void setPort(in_port_t port);
    void setIP(const std::string& ip);

    // unit: millisecond
    // <= 0: means no timeout (Only available when 'SKULLCPP_EP_F_ORPHAN' be set)
    // >  0: after x milliseconds, the ep call would time out
    void setTimeout(int timeout);

    // Set value of 'SKULLCPP_EP_F_CONCURRENT' and 'SKULLCPP_EP_F_ORPHAN'
    void setFlags(int flags);
    void setUnpack(unpack unpackFunc);

public:
    /**
     * Invoke a EP call which would pending the service api
     *
     * @return
     *    - OK      Everything is ok
     *    - ERROR   No input data or calling outside a service api
     *    - TIMEOUT Timeout occurred
     *
     * @note These two apis only can be called from a service api code block,
     *        otherwise it would return an ERROR status
     */
    Status send(const Service&, const void* data, size_t dataSz, epPendingCb cb);
    Status send(const Service&, const std::string& data, epPendingCb cb);

    /**
     * Invoke a EP call which would *NOT* pending the service api
     *
     * @return
     *    - OK      Everything is ok
     *    - ERROR   No input data
     *    - TIMEOUT Timeout occurred
     */
    Status send(const Service&, const void* data, size_t dataSz, epNoPendingCb cb);
    Status send(const Service&, const std::string& data, epNoPendingCb cb);
};

class EPClientNPRet {
private:
    // Make noncopyable
    EPClientNPRet(const EPClientNPRet&) = delete;
    EPClientNPRet(EPClientNPRet&&) = delete;
    EPClientNPRet& operator=(const EPClientNPRet&) = delete;
    EPClientNPRet& operator=(EPClientNPRet&&) = delete;

public:
    EPClientNPRet() {}
    virtual ~EPClientNPRet() {}

public:
    virtual EPClient::Status status() const = 0;
    virtual int latency() const = 0;
    virtual const void* response() const = 0;
    virtual size_t responseSize() const = 0;
};

class EPClientRet {
private:
    // Make noncopyable
    EPClientRet(const EPClientRet&) = delete;
    EPClientRet(EPClientRet&&) = delete;
    EPClientRet& operator=(const EPClientRet&) = delete;
    EPClientRet& operator=(EPClientRet&&) = delete;

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

