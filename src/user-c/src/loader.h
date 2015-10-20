#ifndef SKULL_C_LOADER_H
#define SKULL_C_LOADER_H

#include <skull/txn.h>
#include <skull/txndata.h>
#include <skull/config.h>

typedef struct skull_c_mdata {
    // dll handler
    void* handler;

    // user data created by `init`
    void* ud;

    // config, it only support 1D format (key: value)
    skull_config_t* config;

    // user layer callback apis
    void*  (*init)    (skull_config_t*);
    int    (*run)     (skull_txn_t* txn);
    size_t (*unpack)  (skull_txn_t* txn, const void* data, size_t data_len);
    void   (*pack)    (skull_txn_t* txn, skull_txndata_t* txndata);
    void   (*release) (void* user_data);
} skull_c_mdata;

#endif

