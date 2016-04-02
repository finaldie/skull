#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <google/protobuf/message.h>

#include <skullcpp/api.h>
#include "skull_protos.h"
#include "config.h"

// ====================== Service Init/Release =================================
static
void skull_service_init(skullcpp::Service& service, const skull_config_t* config)
{
    printf("skull service init\n");

    // Convert skull_config to skull_static_config
    skull_static_config_convert(config);
}

static
void skull_service_release(skullcpp::Service& service)
{
    skull_static_config_destroy();
    printf("skull service release\n");
}

// ====================== Service APIs Calls ===================================
static
void skull_service_getdata(const skullcpp::Service& service,
                           const google::protobuf::Message& request,
                           google::protobuf::Message& response)
{
    printf("skull service api: getdata\n");
    SKULL_LOG_INFO("svc.test.get-1", "service get data");
}

// ====================== Register Service =====================================
static skullcpp::ServiceReadApi api_get = {"get", skull_service_getdata};

static skullcpp::ServiceReadApi* api_read_tbl[] = {
    &api_get,
    NULL
};

static skullcpp::ServiceWriteApi* api_write_tbl[] = {
    NULL
};

static skullcpp::ServiceEntry service_entry = {
    skull_service_init,
    skull_service_release,
    api_read_tbl,
    api_write_tbl
};

SKULLCPP_SERVICE_REGISTER(&service_entry)
