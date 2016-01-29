#ifndef SKULL_MODULE_H
#define SKULL_MODULE_H

#include <skull/txn.h>
#include <skull/txndata.h>
#include <skull/config.h>

typedef struct skull_module_t {
    void* ud;

    // user layer callback apis
    void   (*init)    (void* ud);
    int    (*run)     (void* ud, skull_txn_t* txn);
    size_t (*unpack)  (void* ud, skull_txn_t* txn, const void* data, size_t data_len);
    void   (*pack)    (void* ud, skull_txn_t* txn, skull_txndata_t* txndata);
    void   (*release) (void* ud);
} skull_module_t;

#endif

