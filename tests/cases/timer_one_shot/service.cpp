#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <iostream>
#include <google/protobuf/message.h>

#include <skullcpp/api.h>
#include "skull_protos.h"
#include "config.h"

static
void _timerjob(skullcpp::Service& service, skullcpp::ServiceApiData& apiData) {
    std::cout << "timer job triggered" << std::endl;
    SKULL_LOG_INFO("svc.test.timerjob", "timer job triggered");
}

static
void _timerjob_np(skullcpp::Service& service) {
    std::cout << "no pending timer job triggered" << std::endl;
    SKULL_LOG_INFO("svc.test.timerjob_np", "timer job triggered1");
}

// ====================== Service Init/Release =================================
static
void skull_service_init(skullcpp::Service& service, const skull_config_t* config)
{
    printf("skull service init\n");

    skullcpp::Config::instance().load(config);

    // Create a timer job (should be failed)
    int ret = service.createJob(1000, skull_BindSvcJob(_timerjob));
    if (!ret) {
        SKULLCPP_LOG_INFO("init", "service job create successful");
    } else {
        SKULLCPP_LOG_ERROR("init", "service job create failed", "should check parameters");
    }

    // Try again
    ret = service.createJob(1000, 0, skull_BindSvcJobNP(_timerjob_np));
    if (!ret) {
        SKULLCPP_LOG_INFO("init1", "service job create successful1");
    } else {
        SKULLCPP_LOG_ERROR("init1", "service job create failed1", "should check parameters");
    }
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
