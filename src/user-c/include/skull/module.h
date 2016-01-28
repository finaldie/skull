#ifndef SKULL_MODULE_H
#define SKULL_MODULE_H

#include <skull/txn.h>
#include <skull/txndata.h>
#include <skull/config.h>

typedef struct skull_module_t {
    // config, it only support 1D format (key: value)
    const skull_config_t* config;

    // user layer callback apis
    void   (*init)    (const skull_config_t*);
    int    (*run)     (skull_txn_t* txn);
    size_t (*unpack)  (skull_txn_t* txn, const void* data, size_t data_len);
    void   (*pack)    (skull_txn_t* txn, skull_txndata_t* txndata);
    void   (*release) ();
} skull_module_t;

#endif

