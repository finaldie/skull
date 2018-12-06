#ifndef SK_MODULE_H
#define SK_MODULE_H

#include <stddef.h>
#include <stdio.h>
#include "api/sk_config.h"
#include "api/sk_malloc.h"

struct sk_txn_t;

typedef struct sk_module_stat_t {
    size_t unpack;
    size_t pack;
    size_t run;
} sk_module_stat_t;

typedef struct sk_module_t {
    void* md;     // module data
    const sk_module_cfg_t* cfg;

    // stat data (dynamic)
    sk_module_stat_t stat;

    // Interfaces
    int     (*init)   (void* md);
    int     (*run)    (void* md, struct sk_txn_t* txn);
    ssize_t (*unpack) (void* md, struct sk_txn_t* txn,
                       const void* data, size_t data_len);
    int     (*pack)   (void* md, struct sk_txn_t* txn);
    void    (*release)(void* md);
} sk_module_t;

// Atomic Interfaces
size_t sk_module_stat_inc_unpack(sk_module_t*);
size_t sk_module_stat_inc_pack(sk_module_t*);
size_t sk_module_stat_inc_run(sk_module_t*);

#endif

