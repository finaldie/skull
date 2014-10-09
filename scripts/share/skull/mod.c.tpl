#include <stdlib.h>
#include <string.h>

#include "skull/sk_utils.h"
#include "skull/sk_txn.h"

void module_init()
{
    sk_print("module(test): init\n");
}

size_t module_unpack(const char* data, size_t data_sz)
{
    SK_ASSERT(data);
    SK_ASSERT(data_sz);

    sk_print("module_unpack(test): data sz:%zu\n", data_sz);
    return data_sz;
}

int module_run(sk_txn_t* txn)
{
    size_t data_sz = 0;
    const char* data = sk_txn_input(txn, &data_sz);
    char* tmp = calloc(1, data_sz + 1);
    memcpy(tmp, data, data_sz);

    sk_print("receive data: %s\n", tmp);
    free(tmp);
    return 0;
}

void module_pack(sk_txn_t* txn)
{
    size_t data_sz = 0;
    const char* data = sk_txn_input(txn, &data_sz);

    sk_print("module_pack(test): data sz:%zu\n", data_sz);
    sk_txn_output_append(txn, data, data_sz);
}
