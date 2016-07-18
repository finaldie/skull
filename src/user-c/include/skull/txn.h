#ifndef SKULL_TXN_H
#define SKULL_TXN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct _skull_txn_t skull_txn_t;

// get idl name from txn
const char* skull_txn_idlname(const skull_txn_t* txn);

typedef enum skull_txn_status_t {
    SKULL_TXN_OK = 0,
    SKULL_TXN_ERROR
} skull_txn_status_t;

// get txn status
skull_txn_status_t skull_txn_status(const skull_txn_t* txn);

void* skull_txn_data(const skull_txn_t* skull_txn);

void skull_txn_setdata(skull_txn_t* skull_txn, const void* data);

#ifdef __cplusplus
}
#endif

#endif

