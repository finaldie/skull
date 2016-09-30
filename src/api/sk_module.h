#ifndef SK_MODULE_H
#define SK_MODULE_H

#include <stddef.h>
#include <stdio.h>
#include "api/sk_config.h"

struct sk_txn_t;

typedef struct sk_module_t {
    void* md;     // module data
    const sk_module_cfg_t* cfg;

    void    (*init)   (void* md);
    int     (*run)    (void* md, struct sk_txn_t* txn);
    ssize_t (*unpack) (void* md, struct sk_txn_t* txn,
                       const void* data, size_t data_len);
    void    (*pack)   (void* md, struct sk_txn_t* txn);
    void    (*release)(void* md);
} sk_module_t;

#endif

