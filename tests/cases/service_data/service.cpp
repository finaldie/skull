#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <string>
#include <google/protobuf/message.h>

#include <skullcpp/api.h>
#include "skull_protos.h"
#include "config.h"

// ====================== Service Data ======================
class TestData : public skullcpp::ServiceData {
private:
    std::string data_;

public:
    TestData() {}
    virtual ~TestData() {}

public:
    const std::string& get() const {
        return this->data_;
    }

    void set(const std::string& data) {
        this->data_ = data;
    }
};

// ====================== Service Init/Release =================================
static
int  skull_service_init(skullcpp::Service& service, const skull_config_t* config)
{
    printf("skull service init\n");
    skullcpp::Config::instance().load(config);

    auto* testData = new TestData();
    service.set(testData);
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

    const auto* testData = (const TestData*)service.get();
    auto& apiResp = (skull::service::s1::get_resp&)response;
    apiResp.set_response("Hi new bie, " + testData->get());
}

static
void skull_service_setdata(skullcpp::Service& service,
                           const google::protobuf::Message& request,
                           google::protobuf::Message& response)
{
    printf("skull service api: setdata\n");
    SKULL_LOG_INFO("svc.test.set-1", "service set data");

    const auto& apiReq = (const skull::service::s1::set_req&)request;
    auto& apiResp = (skull::service::s1::set_resp&)response;

    auto* testData = (TestData*)service.get();
    std::cout << "set data: " << apiReq.name() << std::endl;
    testData->set(apiReq.name());
    SKULL_LOG_INFO("svc.test.set-2", "service set data done: %s", testData->get().c_str());
    apiResp.set_response("OK");
}

// ====================== Register Service =====================================
static skullcpp::ServiceReadApi api_read_tbl[] = {
    {"get", skull_service_getdata},
    {NULL, NULL}
};

static skullcpp::ServiceWriteApi api_write_tbl[] = {
    {"set", skull_service_setdata},
    {NULL, NULL}
};

static skullcpp::ServiceEntry service_entry = {
    skull_service_init,
    skull_service_release,
    api_read_tbl,
    api_write_tbl
};

SKULLCPP_SERVICE_REGISTER(&service_entry)
