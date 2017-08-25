#ifndef SKULLCPP_SVC_UTILS_H
#define SKULLCPP_SVC_UTILS_H

#include <stdlib.h>
#include <google/protobuf/message.h>

#include <string>
#include <skull/service.h>
#include <skull/unittest.h>
#include <skullcpp/service.h>
#include <skullcpp/service_loader.h>

namespace skullcpp {

template<class T>
T* FindApi(T* apis, const char* api_name) {
    if (!apis || !api_name) return NULL;

    for (int i = 0; apis[i].name != NULL; i++) {
        T* api = &apis[i];

        if (0 == strcmp(api->name, api_name)) {
            return api;
        }
    }

    return NULL;
}

class ServiceApiReqData {
private:
    skull_service_t* svc_;
    std::string      apiName_;
    std::string      svcName_;
    std::string      descName_;
    google::protobuf::Message* msg_;

    bool destroyMsg_;

    uint32_t __padding :24;
    uint32_t __padding1;

private:
    void deserializeMsg(const void* data, size_t sz);

public:
    ServiceApiReqData(skull_service_t* svc, const std::string& apiName);

    ServiceApiReqData(const std::string& svcName, const std::string& apiName,
                      const google::protobuf::Message& msg);

    ServiceApiReqData(const std::string& svcName, const std::string& apiName,
                      const void* data, size_t sz);

    ServiceApiReqData(const skullmock_task_t*);

    ~ServiceApiReqData();

public:
    const google::protobuf::Message& get() const;
    void* serialize(size_t& sz);
};

class ServiceApiRespData {
private:
    skull_service_t* svc_;
    google::protobuf::Message* msg_;
    std::string      descName_;
    bool             storeBack_;

    uint32_t __padding :24;
    uint32_t __padding1;

private:
    void deserializeMsg(const void* data, size_t sz);

public:
    ServiceApiRespData(skull_service_t* svc, const std::string& apiName, bool storeBack);
    ServiceApiRespData(const std::string& svcName, const std::string& apiName,
                       const void* data, size_t sz);
    ServiceApiRespData(skull_service_t* svc, const std::string& apiName,
                       const void* data, size_t sz);
    ~ServiceApiRespData();

public:
    const google::protobuf::Message& get() const;
    google::protobuf::Message& get();
};

} // End of namespace

#endif

