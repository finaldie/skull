#ifndef SKULL_MODULE_EXECUTOR_H
#define SKULL_MODULE_EXECUTOR_H

#include "api/sk_module.h"
#include "api/sk_txn.h"
#include <skull/txn.h>

typedef struct sk_c_mdata {
    void* handler;

    // user layer callback apis
    void   (*init)   ();
    int    (*run)    (skull_txn_t* txn);
    size_t (*unpack) (skull_txn_t* txn, const void* data, size_t data_len);
    void   (*pack)   (skull_txn_t* txn);
} sk_c_mdata;

typedef struct skull_idl_data_t {
    void*  data;
    size_t data_sz;
} skull_idl_data_t;

void   skull_module_init   (void* md);
int    skull_module_run    (void* md, sk_txn_t*);
size_t skull_module_unpack (void* md, sk_txn_t* txn,
                            const void* data, size_t data_len);
void   skull_module_pack   (void* md, sk_txn_t*);

#endif

