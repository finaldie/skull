#ifndef SKULL_TXN_TYPES_H
#define SKULL_TXN_TYPES_H

#include "api/sk_txn.h"

struct _skull_txn_t {
    sk_txn_t* txn;
    void* idl;
};

#endif

