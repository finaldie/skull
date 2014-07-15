#ifndef SK_TYPES_H
#define SK_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef struct sk_ud_t {
    union data {
       uint32_t u32;
       uint64_t u64;
       void*    ud;
    } d;

    size_t sz;
} sk_ud_t;

#endif

