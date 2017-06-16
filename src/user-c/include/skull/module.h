#ifndef SKULL_MODULE_H
#define SKULL_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <skull/txn.h>
#include <skull/txndata.h>
#include <skull/config.h>

typedef struct skull_module_t {
    void* ud;

    // user layer callback apis
    int     (*init)    (void* ud);
    int     (*run)     (void* ud, skull_txn_t* txn);
    ssize_t (*unpack)  (void* ud, skull_txn_t* txn, const void* data, size_t data_len);
    int     (*pack)    (void* ud, skull_txn_t* txn, skull_txndata_t* txndata);
    void    (*release) (void* ud);
} skull_module_t;

#ifdef __cplusplus
}
#endif

#endif

