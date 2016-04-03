#ifndef SKULLCPP_SERVICE_REGISTER_H
#define SKULLCPP_SERVICE_REGISTER_H

#include <google/protobuf/message.h>
#include <skullcpp/service.h>

namespace skullcpp {

#define SERVICE_PROTO_MAXLEN 64

// Notes:
//  - request proto name is 'service-name_req'
//  - response proto name is 'service-name_resp'
typedef struct ServiceReadApi {
    const char* name;

    void (*iocall) (const Service&,
                    const google::protobuf::Message& request,
                    google::protobuf::Message& response);
} ServiceReadApi;

typedef struct ServiceWriteApi {
    const char* name;

    void (*iocall) (Service&,
                    const google::protobuf::Message& request,
                    google::protobuf::Message& response);
} ServiceWriteApi;

typedef struct ServiceEntry {
    void (*init)    (Service&, const skull_config_t*);
    void (*release) (Service&);

    ServiceReadApi*  rApis;
    ServiceWriteApi* wApis;
} ServiceEntry;

} // End of nsmespace

#define SKULLCPP_SERVICE_REGISTER(entry) \
    extern "C" { \
        skullcpp::ServiceEntry* skullcpp_service_register() { \
            return entry; \
        } \
    }

#endif

