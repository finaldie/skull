#ifndef SK_TYPES_H
#define SK_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef union sk_ud_t {
    uint32_t u32;
    uint64_t u64;
    void*    ud;
} sk_ud_t;

typedef unsigned long int ulong_t;

#endif

