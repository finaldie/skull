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

/**
 * Module Initialization Function.
 *
 * @note This function be called during the starting phase before the server
 *        is ready for serving
 */
static
void module_init(const skull_config_t* config)
{
    std::cout << "module(test): init" << std::endl;
    SKULLCPP_LOG_INFO("1", "module(test): init");

    // Load skull_config to skullcpp::Config
    auto& conf = skullcpp::Config::instance();
    conf.load(config);

    // Log the config item values (Feel free to remove them)
    SKULLCPP_LOG_DEBUG("config test_item: " << conf.test_item());
    SKULLCPP_LOG_DEBUG("config test_rate: " << conf.test_rate());
    SKULLCPP_LOG_DEBUG("config test_name: " << conf.test_name());
    SKULLCPP_LOG_DEBUG("config test_bool: " << conf.test_bool());
}

/**
 * Module Release Function
 *
 * @note This function be called during the server shutdown phase
 */
static
void module_release()
{
    std::cout << "module(test): released" << std::endl;
    SKULLCPP_LOG_INFO("5", "module(test): released");
}

/**
 * Unpack the request from raw bytes stream.
 *
 * @note This function only be executed when this module is the first module
 *        in the workflow
 *
 * @return
 *   - = 0: Still need more data
 *   - > 0: Already unpacked a completed request, the returned value will be consumed
 *   - < 0: Error occurred
 */
static
ssize_t module_unpack(skullcpp::Txn& txn, const void* data, size_t data_sz)
{
    // Increase the 'module.request' metrics counter
    moduleMetrics.request.inc(1);

    std::cout << "module_unpack(test): data sz: " << data_sz << std::endl;
    SKULLCPP_LOG_INFO("2", "module_unpack(test): data sz:" << data_sz);

    // Deserialize data to transcation data
    auto& example = (skull::workflow::example&)txn.data();
    example.set_data(data, data_sz);
    return (ssize_t)data_sz;
}

/**
 * Module runnable function, when the transaction is moved to this module, this
 *  function will be executed.
 *
 * @return
 *  -     0: Everything is ok, transation will move to next module
 *  - non-0: Error occurred, transcation will be cancelled, and will run the
 *          `pack` function of the last module
 */
static
int module_run(skullcpp::Txn& txn)
{
    auto& example = (skull::workflow::example&)txn.data();

    // Print and Log the current shared data value
    std::cout << "receive data: " << example.data() << std::endl;
    SKULLCPP_LOG_INFO("3", "receive data: " << example.data());
    return 0;
}

/**
 * Pack the data to `TxnData`, the data will be sent back to the sender. This
 *  function only be called when this module is the last module in the workflow
 */
static
void module_pack(skullcpp::Txn& txn, skullcpp::TxnData& txndata)
{
    auto& example = (skull::workflow::example&)txn.data();

    // If the transcation status is not OK, return a 'error' message
    if (txn.status() != skullcpp::Txn::TXN_OK) {
        SKULLCPP_LOG_ERROR("5", "module_pack(test): error status occurred: "
                           << txn.status() << ". txn data: "
                           << example.data(), "This error is expected");
        txndata.append("error");
        return;
    }

    // Increase the 'module.response' metrics counter
    moduleMetrics.response.inc(1);

    // Log and return response message
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
