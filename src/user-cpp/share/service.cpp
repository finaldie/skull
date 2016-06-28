#include <stdlib.h>

#include <iostream>
#include <string>
#include <google/protobuf/message.h>

#include <skullcpp/api.h>
#include "skull_protos.h"
#include "config.h"

// ====================== Service Init/Release =================================
/**
 * Initialize the service
 */
static
void skull_service_init(skullcpp::Service& service, const skull_config_t* config)
{
    std::cout << "skull service init" << std::endl;

    // Convert skull_config to skull_static_config
    skull_static_config_convert(config);
}

static
void skull_service_release(skullcpp::Service& service)
{
    skull_static_config_destroy();
    std::cout << "skull service release" << std::endl;
}

// ====================== Service APIs Calls ===================================
/**
 * Service API implementation. For the api which has read access, you can call
 *  `service.get()` to fetch the service data. For the api which has write
 *  access, you can also call the `service.set()` to store your service data.
 *
 * @note  DO NOT carry over the service data to another place, the only safe
 *        place for it is leaving it inside the service api scope
 */
static
void skull_service_getdata(const skullcpp::Service& service,
                           const google::protobuf::Message& request,
                           google::protobuf::Message& response)
{
    std::cout << "skull service api: getdata" << std::endl;
    SKULLCPP_LOG_INFO("svc.test.get-1", "service get data");

    auto& apiResp = (skull::service::{SERVICE_NAME}::get_resp&)response;
    apiResp.set_response("Hi new bie");
}

// ====================== Register Service =====================================
static skullcpp::ServiceReadApi api_read_tbl[] = {
    {"get", skull_service_getdata},
    {NULL, NULL}
};

static skullcpp::ServiceWriteApi api_write_tbl[] = {
    {NULL, NULL}
};

static skullcpp::ServiceEntry service_entry = {
    skull_service_init,
    skull_service_release,
    api_read_tbl,
    api_write_tbl
};

SKULLCPP_SERVICE_REGISTER(&service_entry)
