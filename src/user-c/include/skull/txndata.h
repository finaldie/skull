#ifndef SKULL_TXN_DATA_H
#define SKULL_TXN_DATA_H

#include <stddef.h>

typedef struct skull_txndata_t skull_txndata_t;

// get the txn data from input buffer
const void* skull_txndata_input(skull_txndata_t*, size_t* buffer_size);

// append data to txn output buffer
void skull_txndata_output_append(skull_txndata_t*, const void* data, size_t sz);

#endif

