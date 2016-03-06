#ifndef SKULLCPP_SVC_UTILS_H
#define SKULLCPP_SVC_UTILS_H

#include <google/protobuf/message.h>

#include <string>
#include <skull/service.h>
#include <skull/unittest.h>
#include <skullcpp/service.h>
#include <skullcpp/service_loader.h>

namespace skullcpp {

ServiceApi* findApi(ServiceApi** apis, const char* api_name);

class ServiceApiReqRawData {
public:
    void*  data;
    size_t sz;

    std::string    svcName;
    std::string    apiName;
    Service::ApiCB cb;

public:
    ServiceApiReqRawData() : data(NULL), sz(0), cb(NULL) {}
    ~ServiceApiReqRawData() {}
};

class ServiceApiReqData {
private:
    skull_service_t* svc_;
    std::string      apiName_;
    std::string      svcName_;
    std::string      descName_;
    google::protobuf::Message* msg_;

    ServiceApiReqRawData rawData_;
    bool destroyMsg_;

#if __WORDSIZE == 64
    uint32_t __padding :24;
    uint32_t __padding1;
#else
    uint32_t __padding :24;
#endif

private:
    void deserializeMsg(const void* data, size_t sz);

public:
    ServiceApiReqData(skull_service_t* svc, const char* apiName);
    ServiceApiReqData(const ServiceApiReqRawData* rawData);
    ServiceApiReqData(const char* svcName, const char* apiName, const google::protobuf::Message& msg,
                      Service::ApiCB cb);
    ServiceApiReqData(const skullmock_task_t*);

    ~ServiceApiReqData();

public:
    const google::protobuf::Message& get() const;
    ServiceApiReqRawData* serialize(size_t& sz);
};

class ServiceApiRespData {
private:
    skull_service_t* svc_;
    google::protobuf::Message* msg_;
    std::string      descName_;
    bool             storeBack_;

#if __WORDSIZE == 64
    uint32_t __padding :24;
    uint32_t __padding1;
#else
    uint32_t __padding :24;
#endif

private:
    void deserializeMsg(const void* data, size_t sz);

public:
    ServiceApiRespData(skull_service_t* svc, const char* apiName, bool storeBack);
    ServiceApiRespData(const char* svcName, const char* apiName,
                       const void* data, size_t sz);
    ServiceApiRespData(skull_service_t* svc, const char* apiName,
                       const void* data, size_t sz);
    ~ServiceApiRespData();

public:
    const google::protobuf::Message& get() const;
    google::protobuf::Message& get();
};

} // End of namespace

#endif

