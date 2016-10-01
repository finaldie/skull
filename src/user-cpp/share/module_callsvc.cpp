#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <string>
#include <iostream>

#include <skullcpp/api.h>
#include "skull_metrics.h"
#include "skull_protos.h"
#include "config.h"

// Create Metrics
static skullcpp::metrics::module moduleMetrics;

static
void module_init(const skull_config_t* config)
{
    std::cout << "module(test): init" << std::endl;
    SKULLCPP_LOG_TRACE("skull trace log test " << 1);
    SKULLCPP_LOG_DEBUG("skull debug log test " << 2);
    SKULLCPP_LOG_INFO("1", "skull info log test " << 3);
    SKULLCPP_LOG_WARN("1", "skull warn log test " << 4, "ignore, this is test");
    SKULLCPP_LOG_ERROR("1", "skull error log test " << 5, "ignore, this is test");
    SKULLCPP_LOG_FATAL("1", "skull fatal log test " << 6, "ignore, this is test");

    // Load skull_config to skullcpp::Config
    skullcpp::Config::instance().load(config);

    SKULLCPP_LOG_DEBUG("config test_item: " << skullcpp::Config::instance().test_item());
    SKULLCPP_LOG_DEBUG("config test_rate: " << skullcpp::Config::instance().test_rate());
    SKULLCPP_LOG_DEBUG("config test_name: " << skullcpp::Config::instance().test_name());
}

static
void module_release()
{
    std::cout << "module(test): released" << std::endl;
    SKULLCPP_LOG_INFO("5", "module(test): released");
}

static
ssize_t module_unpack(skullcpp::Txn& txn, const void* data, size_t data_sz)
{
    moduleMetrics.request.inc(1);

    std::cout << "module_unpack(test): data sz: " << data_sz << std::endl;
    SKULLCPP_LOG_INFO("2", "module_unpack(test): data sz:" << data_sz);

    // deserialize data to transcation data
    auto& example = (skull::workflow::example&)txn.data();
    example.set_data(data, data_sz);
    return (ssize_t)data_sz;
}

static
int svc_api_callback(skullcpp::Txn& txn, skullcpp::Txn::IOStatus status,
                     const std::string& apiName,
                     const google::protobuf::Message& request,
                     const google::protobuf::Message& response)
{
    if (status != skullcpp::Txn::IOStatus::OK) {
        SKULLCPP_LOG_ERROR("api_callback", "iocall error, status: " << status,
                           "Please increase service max queue limitation");
        return 1;
    }

    const auto& apiReq  = (skull::service::s1::get_req&)request;
    const auto& apiResp = (skull::service::s1::get_resp&)response;

    std::cout << "svc_api_callback.... apiName: " << apiName << std::endl;
    std::cout << "svc_api_callback.... request: " << apiReq.name() << std::endl;
    std::cout << "svc_api_callback.... response: " << apiResp.response() << std::endl;
    return 0;
}

static
int module_run(skullcpp::Txn& txn)
{
    auto& example = (skull::workflow::example&)txn.data();

    std::cout << "receive data: " << example.data() << std::endl;
    SKULLCPP_LOG_INFO("3", "receive data: " << example.data());

    // Call service
    skull::service::s1::get_req req;
    req.set_name("hello service");
    skullcpp::Txn::IOStatus ret =
        txn.iocall("s1", "get", req, 0, svc_api_callback);

    std::cout << "ServiceCall ret: " << ret << std::endl;
    return 0;
}

static
void module_pack(skullcpp::Txn& txn, skullcpp::TxnData& txndata)
{
    auto& example = (skull::workflow::example&)txn.data();

    if (txn.status() != skullcpp::Txn::TXN_OK) {
        SKULLCPP_LOG_ERROR("5", "module_pack(test): error status occurred: "
                           << txn.status() << ". txn data: "
                           << example.data(), "This error is expected");
        txndata.append("error");
        return;
    }

    moduleMetrics.response.inc(1);

    std::cout << "module_pack(test): data sz: " << example.data().length() << std::endl;
    SKULLCPP_LOG_INFO("4", "module_pack(test): data sz:" << example.data().length());
    txndata.append(example.data());
}

/******************************** Register Module *****************************/
static skullcpp::ModuleEntry module_entry = {
    module_init,
    module_release,
    module_run,
    module_unpack,
    module_pack
};

SKULLCPP_MODULE_REGISTER(&module_entry)
