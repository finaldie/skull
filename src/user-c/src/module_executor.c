#include <stdlib.h>
#include "flibs/compiler.h"

#include "api/sk_workflow.h"
#include "api/sk_utils.h"
#include "txn_utils.h"
#include "idl_internal.h"
#include "loader.h"

#include "module_executor.h"

void   skull_module_init   (void* md)
{
    skull_c_mdata* mdata = md;
    mdata->ud = mdata->init(mdata->config);
}

void   skull_module_release(void* md)
{
    skull_c_mdata* mdata = md;
    mdata->release(mdata->ud);
}

int    skull_module_run    (void* md, sk_txn_t* txn)
{
    // 1. deserialize the binary data to user layer structure
    skull_txn_t skull_txn;
    skull_txn_init(&skull_txn, txn);

    // 2. run module
    skull_c_mdata* mdata = md;
    int ret = mdata->run(&skull_txn);

    // 3. serialize the user layer structure back to the binary data
    skull_txn_release(&skull_txn, txn);

    return ret;
}

size_t skull_module_unpack (void* md, sk_txn_t* txn,
                            const void* data, size_t data_len)
{
    SK_ASSERT(sk_txn_udata(txn) == NULL);

    // 1. prepare a fresh new user layer idl structure
    skull_txn_t skull_txn;
    skull_txn_init(&skull_txn, txn);

    // 2. run unpack
    skull_c_mdata* mdata = md;
    size_t consumed_sz = mdata->unpack(&skull_txn, data, data_len);

    // 3. serialize the user layer idl data to binary data
    skull_txn_release(&skull_txn, txn);
    return consumed_sz;
}

void   skull_module_pack   (void* md, sk_txn_t* txn)
{
    SK_ASSERT(sk_txn_udata(txn));

    // 1. deserialize the binary data to user layer structure
    skull_txn_t skull_txn;
    skull_txn_init(&skull_txn, txn);

    // 2. run pack
    skull_txndata_t skull_txndata = {
        .txn = txn
    };

    skull_c_mdata* mdata = md;
    mdata->pack(&skull_txn, &skull_txndata);

    // 3. destroy the user layer structure and the binary data
    protobuf_c_message_free_unpacked(skull_txn.idl, NULL);

    skull_idl_data_t* idl_data = sk_txn_udata(txn);
    free(idl_data->data);
    free(idl_data);
    sk_txn_setudata(txn, NULL);
}
