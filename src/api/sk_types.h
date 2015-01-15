#ifndef SK_TYPES_H
#define SK_TYPES_H

#include <stdint.h>
#include <stddef.h>

#define SK_FILENAME_LEN 256

typedef union sk_ud_t {
    uint32_t u32;
    uint64_t u64;
    void*    ud;
} sk_ud_t;

typedef enum sk_workflow_type_t {
    SK_WORKFLOW_MAIN = 0,
    SK_WORKFLOW_TRIGGER
} sk_workflow_type_t;

#endif

