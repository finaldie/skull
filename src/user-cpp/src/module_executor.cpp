#include <stdlib.h>

#include "skull/txn.h"

#include "txn_idldata.h"
#include "mod_loader.h"
#include "txndata_imp.h"
#include "txn_imp.h"
#include "module_executor.h"

namespace skullcpp {

void   skull_module_init   (void* md)
{
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    entry->init(mdata->config);
}

void   skull_module_release(void* md)
{
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    entry->release();
}

int    skull_module_run    (void* md, skull_txn_t* txn)
{
    // 1. deserialize the binary data to user layer structure
    skullcpp::TxnImp uTxn(txn);

    // 2. run module
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    int ret = entry->run(uTxn);
    return ret;
}

ssize_t skull_module_unpack (void* md, skull_txn_t* txn,
                         const void* data, size_t data_len)
{
    // 1. prepare a fresh new user layer idl structure
    skullcpp::TxnImp uTxn(txn);

    // 2. run unpack
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    ssize_t consumed_sz = entry->unpack(uTxn, data, data_len);
    return consumed_sz;
}

void   skull_module_pack   (void* md, skull_txn_t* txn,
                            skull_txndata_t* txndata)
{
    // 1. Create txn and txndata
    skullcpp::TxnImp uTxn(txn, true);
    skullcpp::TxnDataImp uTxnData(txndata);

    // 2. run pack
    module_data_t* mdata = (module_data_t*)md;
    ModuleEntry* entry = mdata->entry;

    entry->pack(uTxn, uTxnData);
}

} // End of namespace

