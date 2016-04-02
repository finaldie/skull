#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <iostream>
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

static
size_t _ep_unpack(void* ud, const void* data, size_t len)
{
    std::cout << "ep received data len: " << len << std::endl;
    return len;
}

static
void _ep_release(void* ud)
{
    std::cout << "ep released" << std::endl;
}

static
void _ep_cb(skullcpp::Service&, skullcpp::EPClient::Ret ret,
            const void* response, size_t len, void* ud,
            const google::protobuf::Message& apiRequest,
            google::protobuf::Message& apiResponse)
{
    std::cout << "in _ep_cb data len: " << len << std::endl;
    apiRequest.PrintDebugString();
    std::string rawResponse = ((s1::get_resp&)apiResponse).response();
    ((s1::get_resp&)apiResponse).set_response(rawResponse + " , plus ep");
}

// ====================== Service APIs Calls ===================================
static
void skull_service_getdata(const skullcpp::Service& service,
                           const google::protobuf::Message& request,
                           google::protobuf::Message& response)
{
    printf("skull service api: getdata\n");
    SKULL_LOG_INFO("svc.test.get-1", "service get data");

    std::cout << "api req: " << ((s1::get_req&)request).name() << std::endl;
    ((s1::get_resp&)response).set_response("Hi new bie");

    skullcpp::EPClient epClient;
    epClient.setType(skullcpp::EPClient::TCP);
    epClient.setIP("127.0.0.1");
    epClient.setPort(7760);
    epClient.setTimeout(50);
    epClient.setUnpack(_ep_unpack);
    epClient.setRelease(_ep_release);

    skullcpp::EPClient::Status st = epClient.send(service, "hello ep", 9, _ep_cb, NULL);
    std::cout << "ep status: " << st << std::endl;
}

// ====================== Register Service =====================================
static skullcpp::ServiceReadApi api_get = {"get", skull_service_getdata};

static skullcpp::ServiceReadApi* api_read_tbl[] = {
    &api_get,
    NULL
};

static skullcpp::ServiceEntry service_entry = {
    skull_service_init,
    skull_service_release,
    api_read_tbl,
    NULL
};

SKULLCPP_SERVICE_REGISTER(&service_entry)
