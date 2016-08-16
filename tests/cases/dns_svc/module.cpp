#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "skullcpp/api.h"
#include "skull_metrics.h"
#include "skull_protos.h"
#include "config.h"

static
void module_init(const skull_config_t* config)
{
    printf("module(test): init\n");
    SKULL_LOG_TRACE("skull trace log test %d", 1);
    SKULL_LOG_DEBUG("skull debug log test %d", 2);
    SKULL_LOG_INFO("1", "skull info log test %d", 3);
    SKULL_LOG_WARN("1", "skull warn log test %d", 4, "ignore, this is test");
    SKULL_LOG_ERROR("1", "skull error log test %d", 5, "ignore, this is test");
    SKULL_LOG_FATAL("1", "skull fatal log test %d", 6, "ignore, this is test");

    skullcpp::Config::instance().load(config);
}

static
void module_release()
{
    printf("module(test): released\n");
    SKULL_LOG_INFO("5", "module(test): released");
}

static
size_t module_unpack(skullcpp::Txn& txn, const void* data, size_t data_sz)
{
    skull_metrics_module.request.inc(1);
    std::cout << "module_unpack(test): data sz: " << data_sz << std::endl;
    SKULL_LOG_INFO("2", "module_unpack(test): data sz:%zu", data_sz);

    // deserialize data to transcation data
    skull::workflow::example& example = (skull::workflow::example&)txn.data();
    example.set_data(data, data_sz);
    return data_sz;
}

static
int _dns_query_cb(skullcpp::Txn& txn, skullcpp::Txn::IOStatus status,
                  const std::string& apiName,
                  const google::protobuf::Message& request,
                  const google::protobuf::Message& response)
{
    std::cout << "dns query result: " << std::endl;
    const skull::service::dns::query_resp& query_resp =
        (const skull::service::dns::query_resp&)response;

    if (!query_resp.code()) {
        // correct
        std::cout << "return ip: " << query_resp.ip() << std::endl;
        SKULLCPP_LOG_INFO("dns_query_cb-1", "query reponse: ip = "
                          << query_resp.ip());
    } else {
        // error ocurred
        std::cout << "dns error reason: " << query_resp.error() << std::endl;
        SKULLCPP_LOG_ERROR("dns_query_cb-2", "query error occurred, reasone: "
            << query_resp.error(), "Network issue?");
    }

    return 0;
}

static
int module_run(skullcpp::Txn& txn)
{
    skull::workflow::example& example = (skull::workflow::example&)txn.data();

    std::cout << "receive data: " << example.data() << std::endl;
    SKULL_LOG_INFO("3", "receive data: %s", example.data().c_str());

    // Call dns service
    skull::service::dns::query_req query_req;
    query_req.set_domain("www.google.com");

    skullcpp::Txn::IOStatus ret =
        txn.serviceCall("dns", "query", query_req, 0, _dns_query_cb);

    std::cout << "ServiceCall ret: " << ret << std::endl;
    return 0;
}

static
void module_pack(skullcpp::Txn& txn, skullcpp::TxnData& txndata)
{
    skull::workflow::example& example = (skull::workflow::example&)txn.data();

    skull_metrics_module.response.inc(1);
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
