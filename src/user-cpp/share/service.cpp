#include <stdlib.h>

#include <iostream>
#include <string>
#include <google/protobuf/message.h>

#include <skullcpp/api.h>
#include "skull_protos.h"
#include "config.h"

// ====================== Service Init/Release =================================
/**
 * Service Initialization. It will be called before module initialization.
 */
static
void skull_service_init(skullcpp::Service& service, const skull_config_t* config)
{
    std::cout << "skull service init" << std::endl;
    SKULLCPP_LOG_INFO("svc.{SERVICE_NAME}", "Skull service initializing");

    // Load skull_config to skullcpp::Config
    skullcpp::Config::instance().load(config);
}

/**
 * Service Release
 */
static
void skull_service_release(skullcpp::Service& service)
{
    std::cout << "skull service release" << std::endl;
    SKULLCPP_LOG_INFO("svc.{SERVICE_NAME}", "Skull service releasd");
}

/**************************** Service APIs Calls *******************************
 *
 * Service API implementation. For the api which has read access, you can call
 *  `service.get()` to fetch the service data. For the api which has write
 *  access, you can also call the `service.set()` to store your service data.
 *
 * @note  DO NOT carry over the service data to another place, the only safe
 *         place for it is leaving it inside the service api scope
 *        API message name is end with '_req' for the request, with '_resp'
 *         for the response.
 */

/**************************** Example Service API ******************************
 *
 * static
 * void skull_service_get(const skullcpp::Service& service,
 *                        const google::protobuf::Message& request,
 *                        google::protobuf::Message& response) {
 *     SKULLCPP_LOG_INFO("svc.{SERVICE_NAME}.get-1", "Get service data");
 *
 *     auto& apiResp = (skull::service::{SERVICE_NAME}::get_resp&)response;
 *     apiResp.set_response("Hi new bie");
 * }
 */

/**************************** Register Service ********************************/
// Register Read APIs Here
static skullcpp::ServiceReadApi api_read_tbl[] = {
    /**
     * Format: {API_Name, API_Entry_Function}, e.g. {"get", skull_service_get}
     */
    {NULL, NULL}
};

// Register Write APIs Here
static skullcpp::ServiceWriteApi api_write_tbl[] = {
    /**
     * Format: {API_Name, API_Entry_Function}, e.g. {"set", skull_service_set}
     */
    {NULL, NULL}
};

// Register Service Entries
static skullcpp::ServiceEntry service_entry = {
    skull_service_init,
    skull_service_release,
    api_read_tbl,
    api_write_tbl
};

SKULLCPP_SERVICE_REGISTER(&service_entry)
