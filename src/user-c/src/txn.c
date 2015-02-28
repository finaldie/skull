#include <stdlib.h>
#include <string.h>

#include "api/sk_workflow.h"
#include "txn_types.h"
#include "skull/txn.h"
#include "skull/txndata.h"

const char* skull_txn_idlname(skull_txn_t* txn)
{
    sk_workflow_t* workflow = sk_txn_workflow(txn->txn);
    const char* idl_name = workflow->cfg->idl_name;
    size_t offset = 0;
    size_t package_name_len = strlen(txn->descriptor->package_name);
    if (package_name_len > 0) {
        offset = package_name_len + 1;
    }

    return idl_name + offset;
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
