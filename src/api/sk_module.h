#ifndef SK_MODULE_H
#define SK_MODULE_H

#include "fmbuf/fmbuf.h"

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
    int (*sk_module_run)();
    int (*sk_module_unpack)(fmbuf* input);
    int (*sk_module_pack)(fmbuf* output);
} sk_module_t;

#endif

