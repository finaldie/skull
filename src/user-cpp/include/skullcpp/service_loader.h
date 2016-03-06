#ifndef SKULLCPP_SERVICE_REGISTER_H
#define SKULLCPP_SERVICE_REGISTER_H

#include <google/protobuf/message.h>
#include <skullcpp/service.h>

namespace skullcpp {

#define SERVICE_PROTO_MAXLEN 64

// Notes:
//  - request proto name is 'service-name_req'
//  - response proto name is 'service-name_resp'
typedef struct ServiceApi {
    const char* name;

    void (*iocall) (Service&,
                    const google::protobuf::Message& request,
                    google::protobuf::Message& response);
} ServiceApi;

typedef struct ServiceEntry {
    void (*init)    (Service&, skull_config_t*);
    void (*release) (Service&);

    ServiceApi** apis;
} ServiceEntry;

} // End of nsmespace

#define SKULLCPP_SERVICE_REGISTER(entry) \
    extern "C" { \
        skullcpp::ServiceEntry* skullcpp_service_register() { \
            return entry; \
        } \
    }

#endif

