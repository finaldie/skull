#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <iostream>
#include <google/protobuf/message.h>

#include <skullcpp/api.h>
#include "skull_protos.h"
#include "config.h"

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
void _ep_cb(const skullcpp::Service& service, skullcpp::EPClientRet& ret)
{
    std::cout << "in _ep_cb data len: " << ret.responseSize() << std::endl;
    SKULL_LOG_INFO("svc.test.ep_cb", "status: %d, responseSize: %zu, latency: %d",
                   ret.status(), ret.responseSize(), ret.latency());

    std::string epResp((const char*)ret.response(), ret.responseSize());
    SKULLCPP_LOG_INFO("svc.test.ep_cb-1", "ep response: " << epResp);
}

static
void _ep_nopending_cb(const skullcpp::Service& service, skullcpp::EPClientNPRet& ret)
{
    std::cout << "in _ep_np_cb data len: " << ret.responseSize() << std::endl;
    SKULL_LOG_INFO("svc.test.ep_np_cb", "status: %d, responseSize: %zu, latency: %d",
                   ret.status(), ret.responseSize(), ret.latency());

    std::string epResp((const char*)ret.response(), ret.responseSize());
    SKULLCPP_LOG_INFO("svc.test.ep_np_cb-1", "ep response: " << epResp);
}

static
void _timerjob(const skullcpp::Service& service, skullcpp::ServiceApiData& apiData) {
    std::cout << "timer job triggered" << std::endl;
    SKULL_LOG_INFO("svc.test.timerjob", "timer job triggered");

    skullcpp::EPClient epClient;
    epClient.setType(skullcpp::EPClient::TCP);
    epClient.setIP("127.0.0.1");
    epClient.setPort(7760);
    epClient.setTimeout(50);
    epClient.setUnpack(skull_BindEpUnpack(_ep_unpack));

    // Call a pending ep call, it would successfully
    int st = epClient.send(service, "hello ep", skull_BindEpCb(_ep_cb));
    std::cout << "ep status: " << st << std::endl;
    SKULLCPP_LOG_INFO("svc.test-get-3", "ep status: " << st);
}

// ====================== Service Init/Release =================================
static
int  skull_service_init(skullcpp::Service& service, const skull_config_t* config)
{
    printf("skull service init\n");
    skullcpp::Config::instance().load(config);
    return 0;
}

static
void skull_service_release(skullcpp::Service& service)
{
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

    const auto& apiReq = (skull::service::s1::get_req&)request;
    auto& apiResp = (skull::service::s1::get_resp&)response;

    std::cout << "api req: " << apiReq.name() << std::endl;
    apiResp.set_response("Hi new bie");

    // Create a timer job (pending, write)
    int ret = service.createJob((uint32_t)1000, skull_BindSvcJobR(_timerjob), NULL);
    SKULLCPP_LOG_INFO("init", "create service job, ret: " << ret);
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
