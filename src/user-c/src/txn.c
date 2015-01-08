#include <stdlib.h>

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
