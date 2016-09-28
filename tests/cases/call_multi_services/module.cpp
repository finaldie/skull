#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>

#include "skullcpp/api.h"
#include "skull_metrics.h"
#include "skull_protos.h"
#include "config.h"

static skullcpp::metrics::module moduleMetrics;

static
void module_init(const skull_config_t* config)
{
    std::cout << "module(test): init" << std::endl;
    SKULL_LOG_TRACE("skull trace log test %d", 1);
    SKULL_LOG_DEBUG("skull debug log test %d", 2);
    SKULL_LOG_INFO("1", "skull info log test %d", 3);
    SKULL_LOG_WARN("1", "skull warn log test %d", 4, "ignore, this is test");
    SKULL_LOG_ERROR("1", "skull error log test %d", 5, "ignore, this is test");
    SKULL_LOG_FATAL("1", "skull fatal log test %d", 6, "ignore, this is test");

    // Load skull_config to skullcpp::Config
    skullcpp::Config::instance().load(config);
}

static
void module_release()
{
    std::cout << "module(test): released" << std::endl;
    SKULL_LOG_INFO("5", "module(test): released");
}

static
size_t module_unpack(skullcpp::Txn& txn, const void* data, size_t data_sz)
{
    moduleMetrics.request.inc(1);

    std::cout << "module_unpack(test): data sz: " << data_sz << std::endl;
    SKULL_LOG_INFO("2", "module_unpack(test): data sz:%zu", data_sz);

    // deserialize data to transcation data
    auto& example = (skull::workflow::example&)txn.data();
    example.set_data(data, data_sz);
    return data_sz;
}

static
int svc_api_callback(skullcpp::Txn& txn, skullcpp::Txn::IOStatus status,
                     const std::string& apiName,
                     const google::protobuf::Message& request,
                     const google::protobuf::Message& response)
{
    const auto& apiReq  = (skull::service::s1::get_req&)request;
    const auto& apiResp = (skull::service::s1::get_resp&)response;

    std::cout << "svc_api_callback.... apiName: " << apiName << std::endl;
    std::cout << "svc_api_callback.... request: " << apiReq.name() << std::endl;
    std::cout << "svc_api_callback.... response: " << apiResp.response() << std::endl;

    auto& example = (skull::workflow::example&)txn.data();
    example.set_data(example.data() + ", " + apiResp.response());
    return 0;
}

static
int module_run(skullcpp::Txn& txn)
{
    auto& example = (skull::workflow::example&)txn.data();

    std::cout << "receive data: " << example.data() << std::endl;
    SKULL_LOG_INFO("3", "receive data: %s", example.data().c_str());

    // Call service
    skull::service::s1::get_req req;
    req.set_name(example.data());

    skullcpp::Txn::IOStatus ret =
        txn.iocall("s1", "get", req, 0, svc_api_callback);

    std::cout << "ServiceCall ret: " << ret << std::endl;

    // Call service again
    skullcpp::Txn::IOStatus ret1 =
        txn.iocall("s1", "get", req, 0, svc_api_callback);

    std::cout << "ServiceCall ret1: " << ret1 << std::endl;
    return 0;
}

static
void module_pack(skullcpp::Txn& txn, skullcpp::TxnData& txndata)
{
    moduleMetrics.response.inc(1);
    auto& example = (skull::workflow::example&)txn.data();

    std::cout << "module_pack(test): data sz: " << example.data().length() << std::endl;
    SKULL_LOG_INFO("4", "module_pack(test): data sz:%zu", example.data().length());
    txndata.append(example.data().c_str(), example.data().length());
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
