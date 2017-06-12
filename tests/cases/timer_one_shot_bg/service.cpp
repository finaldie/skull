#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <iostream>
#include <google/protobuf/message.h>

#include <skullcpp/api.h>
#include "skull_protos.h"
#include "config.h"

static
void _timerjob(skullcpp::Service& service) {
    std::cout << "timer job triggered" << std::endl;
    SKULL_LOG_INFO("svc.test.timerjob", "timer job triggered");
}

// ====================== Service Init/Release =================================
static
int  skull_service_init(skullcpp::Service& service, const skull_config_t* config)
{
    printf("skull service init\n");
    skullcpp::Config::instance().load(config);

    // Create a timer job
    int ret = service.createJob(1000, 1, skull_BindSvcJobNPW(_timerjob), NULL);
    SKULLCPP_LOG_INFO("init", "create service job, ret: " << ret);
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

    auto& apiResp = (skull::service::s1::get_resp&)response;
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
