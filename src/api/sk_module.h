#ifndef SK_MODULE_H
#define SK_MODULE_H

#include "fmbuf/fmbuf.h"

struct sk_txn_t;

typedef enum sk_mtype_t {
    SK_C_MODULE_TYPE = 0
} sk_mtype_t;

typedef struct sk_module_t {
    void*       md;     // module data
    sk_mtype_t  type;

#if __WORDSIZE == 64
    int         padding;
#endif

    int (*sk_module_init)();
    int (*sk_module_run)(struct sk_txn_t* txn);
    int (*sk_module_unpack)(struct sk_txn_t* txn);
    int (*sk_module_pack)(struct sk_txn_t* txn);
} sk_module_t;

#endif

