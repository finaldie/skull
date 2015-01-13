#ifndef SKULL_TXN_H
#define SKULL_TXN_H

#include <stddef.h>

typedef struct _skull_txn_t skull_txn_t;

// get the data from input buffer
const void* skull_txn_input(skull_txn_t*, size_t* buffer_size);

// append data to output buffer
void skull_txn_output_append(skull_txn_t*, const void* data, size_t size);

#endif

