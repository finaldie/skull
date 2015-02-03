#include <stdlib.h>

#include "api/sk_workflow.h"
#include "txn_types.h"
#include "skull/txn.h"

const void* skull_txn_input(skull_txn_t* txn, size_t* buffer_size)
{
    return sk_txn_input(txn->txn, buffer_size);
}

void skull_txn_output_append(skull_txn_t* txn, const void* data, size_t size)
{
    sk_txn_output_append(txn->txn, data, size);
}

const char* skull_txn_idlname(skull_txn_t* txn)
{
    sk_workflow_t* workflow = sk_txn_workflow(txn->txn);
    return workflow->cfg->idl_name;
}

void* skull_txn_data(skull_txn_t* txn)
{
    return txn->idl;
}
