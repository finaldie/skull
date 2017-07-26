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

    typedef std::function<void (const Service&, EPClientRet& ret)>   EpCb;
    typedef std::function<void (const Service&, EPClientNPRet& ret)> EpNPCb;

    typedef std::function<ssize_t (const void* data, size_t len)> UnpackFn;

// Use these macros to make a unpack/epCb easily. And in another hand, we still
//  can use *lambda* instead of them
#define skull_BindEpUnpack(f, ...) \
    (skullcpp::EPClient::UnpackFn) \
    std::bind(f, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)

#define skull_BindEpCb(f, ...) \
    (skullcpp::EPClient::EpCb) \
    std::bind(f, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)

#define skull_BindEpNPCb(f, ...) \
    (skullcpp::EPClient::EpNPCb) \
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

    // Reserved, no effect currently
    void setFlags(int flags);

    // Setup a unpack function
    void setUnpack(UnpackFn unpackFn);

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
    Status send(const Service&, const void* data, size_t dataSz, EpCb cb) const;
    Status send(const Service&, const std::string& data, EpCb cb) const;

    /**
     * Invoke a EP call which would *NOT* pending the service api
     *
     * @return
     *    - OK      Everything is ok
     *    - ERROR   No input data
     *    - TIMEOUT Timeout occurred
     */
    Status send(const Service&, const void* data, size_t dataSz, EpNPCb cb) const;
    Status send(const Service&, const std::string& data, EpNPCb cb) const;

    /**
     * Invoke a EP call which would *NOT* pending the service api
     *
     * @return
     *    - OK      Everything is ok
     *    - ERROR   No input data
     *    - TIMEOUT Timeout occurred
     */
    Status send(const Service&, const void* data, size_t dataSz) const;
    Status send(const Service&, const std::string& data) const;
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
    virtual EPClient::Type type() const = 0;
    virtual in_port_t port() const = 0;
    virtual const std::string& ip() const = 0;
    virtual int timeout() const = 0; // unit: millisecond
    virtual int flags() const = 0;

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
    virtual EPClient::Type type() const = 0;
    virtual in_port_t port() const = 0;
    virtual const std::string& ip() const = 0;
    virtual int timeout() const = 0; // unit: millisecond
    virtual int flags() const = 0;

public:
    virtual EPClient::Status status() const = 0;
    virtual int latency() const = 0;
    virtual const void* response() const = 0;
    virtual size_t responseSize() const = 0;
    virtual ServiceApiData& apiData() = 0;
};

} // End of namespace

#endif

