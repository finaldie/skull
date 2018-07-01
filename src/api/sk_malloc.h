#ifndef SK_MALLOC_H
#define SK_MALLOC_H

#include <stddef.h>

typedef struct sk_mem_stat_t {
    int nmalloc;
    int nfree;
    int ncalloc;
    int nrealloc;
    int nposix_memalign;
    int naligned_alloc;

    size_t alloc_sz;
    size_t dalloc_sz;
    size_t peak_sz;
} sk_mem_stat_t;

/**
 * Get the static stat structure
 */
sk_mem_stat_t* sk_mem_static();

/**
 * Calculate memory usage
 */
size_t sk_mem_allocated(sk_mem_stat_t*);

#endif

