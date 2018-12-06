#ifndef SK_MALLOC_H
#define SK_MALLOC_H

#include <stddef.h>
#include <stdbool.h>

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

void sk_mem_init(const char* workdir,
                 const char* logname,
                 int  log_level,
                 bool stdout_fwd);
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

bool sk_mem_trace_status();
void sk_mem_trace(bool enabled);

#endif

