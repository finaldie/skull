#ifndef SKULLCPP_EP_H
#define SKULLCPP_EP_H

#include <stddef.h>
#include <netinet/in.h>

#include <string>
#include <memory>

#include <skull/service.h>
#include <skull/ep.h>
#include <google/protobuf/message.h>

#include <skullcpp/service.h>

namespace skullcpp {

class EPClientImpl;

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

    typedef struct Ret {
        Status status;
        int    latency;

        Ret(skull_ep_ret_t ret) {
            if (ret.status == SKULL_EP_OK) {
                this->status = OK;
            } else if (ret.status == SKULL_EP_ERROR) {
                this->status = ERROR;
            } else {
                this->status = TIMEOUT;
            }

            this->latency = ret.latency;
        }
    } Ret;

    // Case 1: Be called from a service api call, then user can get api req and
    //          resp from 'apiData'
    // Case 2: Be called from a service job, then the 'apiData' is useless,
    //          and DO NOT try to use it
    typedef void (*epCb)(const Service&, Ret ret,
                         const void* response, size_t len, void* ud,
                         ServiceApiData& apiData);

    typedef size_t (*unpack) (void* ud, const void* data, size_t len);
    typedef void   (*release)(void* ud);

public:
    EPClient();
    ~EPClient();

public:
    void setType(Type type);
    void setPort(in_port_t port);
    void setIP(const std::string& ip);
    void setTimeout(int timeout);
    void setUnpack(unpack unpackFunc);
    void setRelease(release releaseFunc);

public:
    Status send(const Service&, const void* data, size_t dataSz, epCb cb, void* ud);
};

} // End of namespace

#endif

