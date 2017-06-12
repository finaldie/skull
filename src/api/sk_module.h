#ifndef SK_MODULE_H
#define SK_MODULE_H

#include <stddef.h>
#include <stdio.h>
#include "api/sk_config.h"

struct sk_txn_t;

typedef struct sk_module_stat_t {
    size_t called_total;
    size_t called_unpack;
    size_t called_pack;
    size_t called_run;
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
    void    (*pack)   (void* md, struct sk_txn_t* txn);
    void    (*release)(void* md);
} sk_module_t;

// Atomic Interfaces
size_t sk_module_stat_unpack_inc(sk_module_t*);
size_t sk_module_stat_unpack_get(sk_module_t*);

size_t sk_module_stat_pack_inc(sk_module_t*);
size_t sk_module_stat_pack_get(sk_module_t*);

size_t sk_module_stat_run_inc(sk_module_t*);
size_t sk_module_stat_run_get(sk_module_t*);

size_t sk_module_stat_total_get(sk_module_t*);

#endif

