#ifndef SKULL_MODULE_EXECUTOR_H
#define SKULL_MODULE_EXECUTOR_H

#include "api/sk_txn.h"

void   skull_module_init   (void* md);
int    skull_module_run    (void* md, sk_txn_t*);
size_t skull_module_unpack (void* md, sk_txn_t* txn,
                            const void* data, size_t data_len);
void   skull_module_pack   (void* md, sk_txn_t*);
void   skull_module_release(void* md);

#endif

