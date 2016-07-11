#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>

#include "skullcpp/api.h"
#include "skull_metrics.h"
#include "skull_protos.h"
#include "config.h"

static
void module_init(const skull_config_t* config)
{
    std::cout << "module(test): init" << std::endl;
    SKULLCPP_LOG_TRACE("skull trace log test " << 1);
    SKULLCPP_LOG_DEBUG("skull debug log test " << 2);
    SKULLCPP_LOG_INFO("1", "skull info log test " << 3);
    SKULLCPP_LOG_WARN("1", "skull warn log test " << 4, "ignore, this is test");
    SKULLCPP_LOG_ERROR("1", "skull error log test " << 5, "ignore, this is test");
    SKULLCPP_LOG_FATAL("1", "skull fatal log test " << 5, "ignore, this is test");

    // Load skull_config to skullcpp::Config
    auto& conf = skullcpp::Config::instance();
    conf.load(config);

    SKULLCPP_LOG_DEBUG("config test_item: " << conf.test_item());
    SKULLCPP_LOG_DEBUG("config test_rate: " << conf.test_rate());
    SKULLCPP_LOG_DEBUG("config test_name: " << conf.test_name());
    SKULLCPP_LOG_DEBUG("config test_bool: " << conf.test_bool());
}

static
void module_release()
{
    std::cout << "module(test): released" << std::endl;
    SKULLCPP_LOG_INFO("5", "module(test): released");
}

static
size_t module_unpack(skullcpp::Txn& txn, const void* data, size_t data_sz)
{
    skull_metrics_module.request.inc(1);
    std::cout << "module_unpack(test): data sz: " << data_sz << std::endl;
    SKULLCPP_LOG_INFO("2", "module_unpack(test): data sz:" << data_sz);

    // deserialize data to transcation data
    auto& example = (skull::workflow::example&)txn.data();
    example.set_data(data, data_sz);
    return data_sz;
}

static
int module_run(skullcpp::Txn& txn)
{
    auto& example = (skull::workflow::example&)txn.data();

    std::cout << "receive data: " << example.data() << std::endl;
    SKULLCPP_LOG_INFO("3", "receive data: " << example.data());
    return 0;
}

static
void module_pack(skullcpp::Txn& txn, skullcpp::TxnData& txndata)
{
    auto& example = (skull::workflow::example&)txn.data();

    if (txn.status() != skullcpp::Txn::TXN_OK) {
        SKULLCPP_LOG_ERROR("5", "module_pack(test): error status occurred. txn data: "
                            << example.data(), "This error is expected");
        txndata.append("error");
        return;
    }

    skull_metrics_module.response.inc(1);
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
