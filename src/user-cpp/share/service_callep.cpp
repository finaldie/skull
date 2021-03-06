#include <stdlib.h>

#include <iostream>
#include <string>
#include <iostream>
#include <google/protobuf/message.h>

#include <skullcpp/api.h>
#include "skull_protos.h"
#include "config.h"

// ====================== Service Init/Release =================================
static
int  skull_service_init(skullcpp::Service& service, const skull_config_t* config)
{
    std::cout << "skull service init" << std::endl;

    // Load skull_config to skullcpp::Config
    skullcpp::Config::instance().load(config);
    return 0;
}

static
void skull_service_release(skullcpp::Service& service)
{
    std::cout << "skull service release" << std::endl;
}

static
ssize_t _ep_unpack(const void* data, size_t len)
{
    std::cout << "ep received data len: " << len << std::endl;
    return (ssize_t)len;
}

static
void _ep_release(void* ud)
{
    std::cout << "ep released" << std::endl;
}

static
void _ep_cb(const skullcpp::Service&, skullcpp::EPClientRet& ret)
{
    std::cout << "in _ep_cb data len: " << ret.responseSize() << std::endl;

    auto& apiData = ret.apiData();
    apiData.request().PrintDebugString();
    auto& apiResp = (skull::service::s1::get_resp&)apiData.response();

    const std::string& rawResponse = apiResp.response();
    apiResp.set_response(rawResponse + " , plus ep");
}

// ====================== Service APIs Calls ===================================
static
void skull_service_getdata(const skullcpp::Service& service,
                           const google::protobuf::Message& request,
                           google::protobuf::Message& response)
{
    std::cout << "skull service api: getdata" << std::endl;
    SKULLCPP_LOG_INFO("svc.test.get-1", "service get data");

    const auto& apiReq = (skull::service::s1::get_req&)request;
    auto& apiResp = (skull::service::s1::get_resp&)response;

    std::cout << "api req: " << apiReq.name() << std::endl;
    apiResp.set_response("Hi new bie");

    skullcpp::EPClient epClient;
    epClient.setType(skullcpp::EPClient::TCP);
    epClient.setIP("127.0.0.1");
    epClient.setPort(7760);
    epClient.setTimeout(50);
    epClient.setUnpack(skull_BindEpUnpack(_ep_unpack));

    skullcpp::EPClient::Status st =
        epClient.send(service, "hello ep", 9, skull_BindEpCb(_ep_cb));
    std::cout << "ep status: " << st << std::endl;
}

// ====================== Register Service =====================================
static skullcpp::ServiceReadApi api_read_tbl[] = {
    {"get", skull_service_getdata},
    {NULL, NULL}
};

static skullcpp::ServiceEntry service_entry = {
    skull_service_init,
    skull_service_release,
    api_read_tbl,
    NULL
};

SKULLCPP_SERVICE_REGISTER(&service_entry)
