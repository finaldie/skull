#ifndef SKULLCPP_MODULE_EXECUTOR_H
#define SKULLCPP_MODULE_EXECUTOR_H

#include <stdio.h>
#include "skull/api.h"

namespace skullcpp {

int     skull_module_init   (void* md);
int     skull_module_run    (void* md, skull_txn_t*);
ssize_t skull_module_unpack (void* md, skull_txn_t* txn,
                             const void* data, size_t data_len);
void    skull_module_pack   (void* md, skull_txn_t*, skull_txndata_t* txndata);
void    skull_module_release(void* md);

} // End of namespace

#endif

