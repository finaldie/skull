#include <stdlib.h>
#include <string.h>

#include "api/sk_utils.h"
#include "api/sk_workflow.h"
#include "api/sk_txn.h"
#include "txn_utils.h"
#include "skull/txn.h"
#include "skull/txndata.h"

const char* skull_txn_idlname(skull_txn_t* txn)
{
    size_t offset = 0;
    size_t package_name_len = strlen(txn->descriptor->package_name);
    if (package_name_len > 0) {
        offset = package_name_len + 1;
    }

    return txn->descriptor->name + offset;
}

void* skull_txn_idldata(skull_txn_t* txn)
{
    return txn->idl;
}

const void* skull_txndata_input(skull_txndata_t* txndata, size_t* buffer_size)
{
    return sk_txn_input(txndata->txn, buffer_size);
}

void skull_txndata_output_append(skull_txndata_t* txndata, const void* data,
                                 size_t size)
{
    sk_txn_output_append(txndata->txn, data, size);
}

skull_txn_status_t skull_txn_status(skull_txn_t* skull_txn)
{
    sk_txn_t* txn = skull_txn->txn;
    sk_txn_state_t st = sk_txn_state(txn);

    SK_ASSERT(st != SK_TXN_DESTROYED);
    if (st != SK_TXN_ERROR) {
        return SKULL_TXN_OK;
    } else {
        return SKULL_TXN_ERROR;
    }
}
