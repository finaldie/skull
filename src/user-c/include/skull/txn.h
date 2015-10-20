#ifndef SKULL_TXN_H
#define SKULL_TXN_H

#include <stddef.h>

typedef struct _skull_txn_t skull_txn_t;

// get idl name from txn
const char* skull_txn_idlname(skull_txn_t* txn);

// get idl data
void* skull_txn_idldata(skull_txn_t* txn);

#endif

