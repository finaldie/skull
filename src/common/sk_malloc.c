#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <pthread.h>
#include <malloc.h>

#include "flibs/compiler.h"
#include "api/sk_env.h"
#include "api/sk_utils.h"
#include "api/sk_core.h"
#include "api/sk_service.h"
#include "api/sk_malloc.h"

#ifdef SKULL_JEMALLOC_LINKED
# include "jemalloc/jemalloc.h"
# define SK_ALLOCATOR(symbol) je_ ## symbol
static
size_t _get_malloc_sz(void* ptr) {
    return je_malloc_usable_size(ptr);
}
#else
# define SK_ALLOCATOR(symbol) sk_real_ ## symbol

static
size_t _get_malloc_sz(void* ptr) {
    return malloc_usable_size(ptr);
}
#endif

#define SK_REAL_TYPE(symbol) sk_real_ ## symbol ## _t
#define SK_REAL(symbol) _sk_real_ ## symbol

typedef void* (*SK_REAL_TYPE(malloc))  (size_t);
typedef void  (*SK_REAL_TYPE(free))    (void*);
typedef void* (*SK_REAL_TYPE(calloc))  (size_t, size_t);
typedef void* (*SK_REAL_TYPE(realloc)) (void*, size_t);
typedef int   (*SK_REAL_TYPE(posix_memalign)) (void**, size_t, size_t);
typedef void* (*SK_REAL_TYPE(aligned_alloc))  (size_t, size_t);

static SK_REAL_TYPE(malloc)  SK_REAL(malloc)  = NULL;
static SK_REAL_TYPE(free)    SK_REAL(free)    = NULL;
static SK_REAL_TYPE(calloc)  SK_REAL(calloc)  = NULL;
static SK_REAL_TYPE(realloc) SK_REAL(realloc) = NULL;
static SK_REAL_TYPE(posix_memalign) SK_REAL(posix_memalign) = NULL;
static SK_REAL_TYPE(aligned_alloc)  SK_REAL(aligned_alloc)  = NULL;

#define SK_LOAD_SYMBOL(symbol) \
    do { \
        if (SK_REAL(symbol)) break; \
\
        SK_REAL(symbol) = \
            (SK_REAL_TYPE(symbol))(intptr_t)dlsym(RTLD_NEXT, #symbol); \
\
        if (unlikely(!SK_REAL(symbol))) { \
            fprintf(stderr, "Error in loading mem symbol: %s\n", #symbol); \
            abort(); \
        } \
    } while(0)

/**
 * Inner atomic operations. TODO: Use C11 __Atomic to replace all these
 */
#define SK_ATOMIC_INC(v) __sync_add_and_fetch(&(v), 1)
#define SK_ATOMIC_ADD(v, delta) __sync_add_and_fetch(&(v), (delta))
#define SK_ATOMIC_CAS(v, expect, value) \
    __sync_bool_compare_and_swap(&(v), (expect), (value))
#define SK_ATOMIC_UPDATE_PEAKSZ(nstat) \
    do { \
        size_t curr_peak = nstat->peak_sz; \
        size_t curr_used = sk_mem_allocated(nstat); \
        if (curr_peak == curr_used) break; \
        size_t peak_sz   = SK_MAX(curr_peak, curr_used); \
        if (SK_ATOMIC_CAS(nstat->peak_sz, curr_peak, peak_sz)) break; \
    } while(1)

// The mem_stat before thread_env been set up (mainly for counting the stat
//  during the initialization)
static sk_mem_stat_t istat;

static
void _init() {
    /**
     * Initialize all public symbols besides `calloc`, because `dlsym` would
     *  call `calloc` itself. Here we handle loading `calloc` separately.
     *  See `sk_real_calloc`.
     */
    SK_LOAD_SYMBOL(malloc);
    SK_LOAD_SYMBOL(free);
    SK_LOAD_SYMBOL(realloc);
    SK_LOAD_SYMBOL(posix_memalign);
    SK_LOAD_SYMBOL(aligned_alloc);
}

static
void* sk_real_malloc(size_t sz) {
    if (unlikely(!SK_REAL(malloc))) {
        _init();
    }

    return SK_REAL(malloc)(sz);
}

static
void  sk_real_free(void* ptr) {
    if (unlikely(!SK_REAL(free))) {
        _init();
    }

    SK_REAL(free)(ptr);
}

static
void* __hacky_calloc(size_t nmemb, size_t sz) {
    fprintf(stderr, "__hacky_calloc return NULL\n");
    return NULL;
}

/**
 * Here, to make `dlsym` happy, we use a temporary `__hacky_calloc` to trick
 *  it, to allow `dlsym` can be returned when `calloc` return NULL. The whole
 *  sequence flow like this:
 *
 *  caller    calloc    dlsym    calloc  hacky_calloc real_calloc
 *    |-------->|         |         |         |            |
 *    |         |-------->|         |         |            |
 *    |         |         |-------->|         |            |
 *    |         |         |         |-------->|            |
 *    |         |         |         |<--null--|            |
 *    |         |         |<--null--|         |            |
 *    |         |<-symbol-|         |         |            |
 *    |         |----------------------------------------->|
 *    |         |<------------------ptr--------------------|
 *    |<--ptr---|         |         |         |            |
 */
static
void* sk_real_calloc(size_t nmemb, size_t sz) {
    if (unlikely(!SK_REAL(calloc))) {
        SK_REAL(calloc) = __hacky_calloc;

        FMEM_BARRIER();

        SK_REAL(calloc) =
            (SK_REAL_TYPE(calloc))(intptr_t)dlsym(RTLD_NEXT, "calloc");

        if (unlikely(!SK_REAL(calloc))) {
            fprintf(stderr, "Error in loading mem symbol: calloc\n");
            abort();
        }
    }

    return SK_REAL(calloc)(nmemb, sz);
}

static
void* sk_real_realloc(void* ptr, size_t sz) {
    if (unlikely(!SK_REAL(realloc))) {
        _init();
    }

    return SK_REAL(realloc)(ptr, sz);
}

static
int sk_real_posix_memalign(void **memptr, size_t alignment, size_t size) {
    if (unlikely(!SK_REAL(posix_memalign))) {
        _init();
    }

    return SK_REAL(posix_memalign)(memptr, alignment, size);
}

static
void* sk_real_aligned_alloc(size_t alignment, size_t size) {
    if (unlikely(!SK_REAL(aligned_alloc))) {
        _init();
    }

    return SK_REAL(aligned_alloc)(alignment, size);
}

static
sk_mem_stat_t* _get_stat() {
    sk_mem_stat_t* stat = NULL;

    if (!sk_thread_env_ready() || unlikely(!SK_ENV)) {
        stat = &istat;
    } else {
        sk_env_pos_t pos = SK_ENV_POS;

        switch (pos) {
        case SK_ENV_POS_MODULE:
            stat = &((sk_module_t*)(SK_ENV_CURRENT))->mstat;
            break;
        case SK_ENV_POS_SERVICE:
            stat = sk_service_memstat(SK_ENV_CURRENT);
            break;
        case SK_ENV_POS_CORE:
        default:
            stat = &SK_ENV_CORE->mstat;
            break;
        }
    }

    return stat;
}

/**
 * Public APIs
 */
const sk_mem_stat_t* sk_mem_static() {
    return &istat;
}

size_t sk_mem_allocated(const sk_mem_stat_t* stat) {
    volatile size_t alloc_sz  = stat->alloc_sz;
    volatile size_t dalloc_sz = stat->dalloc_sz;

    return alloc_sz > dalloc_sz
        ? alloc_sz - dalloc_sz : 0;
}

/**
 * Override libc, to measure module/service memory usage
 */
void* malloc(size_t sz) {
    void* ptr = SK_ALLOCATOR(malloc)(sz);

    sk_mem_stat_t* stat = _get_stat();
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->nmalloc);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(ptr));
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    return ptr;
}

void free(void* ptr) {
    sk_mem_stat_t* stat = _get_stat();
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->nfree);
        SK_ATOMIC_ADD(stat->dalloc_sz, _get_malloc_sz(ptr));
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    SK_ALLOCATOR(free)(ptr);
}

/**
 * Tricky for `calloc`, `dlsym` would call `calloc` by itself which lead
 *  infinite recursion, fortunately if `calloc` return NULL, dlsym will
 *  fall back to another path to use global symbol which here is fine.
 *  So here we let `calloc` return NULL via a `__hacky_calloc`, then set
 *  up the real symbol, see `sk_real_calloc` for details.
 *
 *  @note The stat.ncalloc would be 1 time bigger than reality, we can adjust
 *         it in admin module. stat.alloc_sz is accurate, since the nullptr
 *         size is 0.
 */
void* calloc(size_t nmemb, size_t sz) {
    void* ptr = SK_ALLOCATOR(calloc)(nmemb, sz);

    sk_mem_stat_t* stat = _get_stat();
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->ncalloc);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(ptr));
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    return ptr;
}

void* realloc(void* ptr, size_t sz) {
    size_t old_sz = _get_malloc_sz(ptr);

    void* nptr = SK_ALLOCATOR(realloc)(ptr, sz);
    size_t new_sz = _get_malloc_sz(nptr);

    sk_mem_stat_t* stat = _get_stat();
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->nrealloc);

        if (unlikely(!ptr)) {
            SK_ATOMIC_ADD(stat->alloc_sz, new_sz);
        } else if (old_sz != new_sz) {
            SK_ATOMIC_ADD(stat->alloc_sz, new_sz);
            SK_ATOMIC_ADD(stat->dalloc_sz, old_sz);
        }

        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    return nptr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    int ret = SK_ALLOCATOR(posix_memalign)(memptr, alignment, size);

    sk_mem_stat_t* stat = _get_stat();
    if (likely(!ret && stat)) {
        SK_ATOMIC_INC(stat->nposix_memalign);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(*memptr));
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    return ret;
}

void* aligned_alloc(size_t alignment, size_t size) {
    void* ptr = SK_ALLOCATOR(aligned_alloc)(alignment, size);

    sk_mem_stat_t* stat = _get_stat();
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->naligned_alloc);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(ptr));
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    return ptr;
}

