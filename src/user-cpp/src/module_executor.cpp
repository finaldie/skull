#include <stdlib.h>

#include "skull/txn.h"
#include "skullcpp/logger.h"

#include "txn_idldata.h"
#include "mod_loader.h"
#include "txndata_imp.h"
#include "txn_imp.h"
#include "module_executor.h"

namespace skullcpp {

int    skull_module_init   (void* md)
{
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    try {
        return entry->init(mdata->config);
    } catch (std::exception& e) {
        SKULLCPP_LOG_ERROR("module.init", "Exception: " << e.what(),
                           "Abort...");
    }

    // Error occurred
    return -1;
}

void   skull_module_release(void* md)
{
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    try {
        entry->release();
    } catch (std::exception& e) {
        SKULLCPP_LOG_ERROR("module.release", "Exception: " << e.what(),
                           "Fix the code.");
    }
}

int    skull_module_run    (void* md, skull_txn_t* txn)
{
    // 1. deserialize the binary data to user layer structure
    skullcpp::TxnImp uTxn(txn);

    // 2. run module
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    try {
        return entry->run(uTxn);
    } catch (std::exception& e) {
        SKULLCPP_LOG_ERROR("module.run", "Exception: " << e.what(),
                           "Transaction abort, check the code");
    }

    // Error occurred
    return -1;
}

ssize_t skull_module_unpack (void* md, skull_txn_t* txn,
                         const void* data, size_t data_len)
{
    // 1. prepare a fresh new user layer idl structure
    skullcpp::TxnImp uTxn(txn);

    // 2. run unpack
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    try {
        return entry->unpack(uTxn, data, data_len);
    } catch (std::exception& e) {
        SKULLCPP_LOG_ERROR("module.unpack", "Exception: " << e.what(),
                           "Transcation abort, check the code");
    }

    // Error occurred
    return -1;
}

int    skull_module_pack   (void* md, skull_txn_t* txn,
                            skull_txndata_t* txndata)
{
    // 1. Create txn and txndata
    skullcpp::TxnImp uTxn(txn, true);
    skullcpp::TxnDataImp uTxnData(txndata);

    // 2. run pack
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    try {
        entry->pack(uTxn, uTxnData);
    } catch (std::exception& e) {
        SKULLCPP_LOG_ERROR("module.pack", "Exception: " << e.what(),
                           "Trascation abort, check the code");
        return 1;
    }

    return 0;
}

} // End of namespace

