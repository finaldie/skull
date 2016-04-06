#include <stdlib.h>

#include "flibs/compiler.h"
#include "api/sk_workflow.h"
#include "txn_utils.h"

void skull_txn_init(struct _skull_txn_t* skull_txn, const sk_txn_t* txn)
{
    // 1. Restore message
    sk_workflow_t* workflow = sk_txn_workflow(txn);
    const char*    idl_name = workflow->cfg->idl_name;

    // 2. fill skull_txn
    skull_txn->txn = (sk_txn_t*) txn;
    skull_txn->idl_name = idl_name;
}

void skull_txn_release(struct _skull_txn_t* skull_txn, sk_txn_t* txn)
{
}
