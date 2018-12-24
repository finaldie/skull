#ifndef SK_MALLOC_H
#define SK_MALLOC_H

#include <stddef.h>
#include <stdbool.h>

#include "api/sk_log.h"

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

void sk_mem_init();
void sk_mem_init_log(const sk_logger_t*);
void sk_mem_destroy();

/**
 * Calculate memory usage
 */
size_t sk_mem_allocated(const sk_mem_stat_t*);

void sk_mem_stat_module_register(const char* name);
void sk_mem_stat_service_register(const char* name);

const sk_mem_stat_t* sk_mem_stat_module (const char* name);
const sk_mem_stat_t* sk_mem_stat_service(const char* name);

/**
 * Get the static stat structure. This structure tracking the memory status if
 *  engine environment is not ready
 */
const sk_mem_stat_t* sk_mem_stat_static();

/**
 * Get the core stat structure. This structure tracking the memory status if
 *  engine environment is not ready
 */
const sk_mem_stat_t* sk_mem_stat_core();

int sk_mem_trace_level();

/**
 * Tracing Level
 *  0: Disable
 *  1: Caller only
 *  (2-N): Caller
 */
void sk_mem_trace(int level);

/**
 * Dump all the mem stats into log
 */
void sk_mem_dump(const char* fmt, ...);

#endif

