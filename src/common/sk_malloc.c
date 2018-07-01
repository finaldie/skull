#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <pthread.h>

#include "flibs/compiler.h"
#include "api/sk_env.h"
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
    return 0;
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
        SK_REAL(symbol) = \
            (SK_REAL_TYPE(symbol))(intptr_t)dlsym(RTLD_NEXT, #symbol); \
\
        if (unlikely(!SK_REAL(symbol))) { \
            fprintf(stderr, "Error in loading mem symbol: %s\n", dlerror()); \
            exit(1); \
        } \
    } while(0)

#define SK_ATOMIC_INC(v) __sync_add_and_fetch(&(v), 1)
#define SK_ATOMIC_ADD(v, delta) __sync_add_and_fetch(&(v), delta)

// The mem_stat before thread_env been set up (mainly for counting the stat
//  during the initialization)
static sk_mem_stat_t istat;

static
void _init() {
    SK_LOAD_SYMBOL(malloc);
    SK_LOAD_SYMBOL(free);
    SK_LOAD_SYMBOL(calloc);
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
void* sk_real_calloc(size_t nmemb, size_t sz) {
    if (unlikely(!SK_REAL(calloc))) {
        _init();
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
    return stat->alloc_sz >= stat->dalloc_sz
        ? stat->alloc_sz - stat->dalloc_sz : 0;
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
    }

    return ptr;
}

void free(void* ptr) {
    sk_mem_stat_t* stat = _get_stat();
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->nfree);
        SK_ATOMIC_ADD(stat->dalloc_sz, _get_malloc_sz(ptr));
    }

    SK_ALLOCATOR(free)(ptr);
}

void* calloc(size_t nmemb, size_t sz) {
    void* ptr = SK_ALLOCATOR(calloc)(nmemb, sz);

    sk_mem_stat_t* stat = _get_stat();
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->ncalloc);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(ptr));
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
    }

    return nptr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    int ret = SK_ALLOCATOR(posix_memalign)(memptr, alignment, size);

    sk_mem_stat_t* stat = _get_stat();
    if (likely(!ret && !memptr && stat)) {
        SK_ATOMIC_INC(stat->nposix_memalign);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(*memptr));
    }

    return ret;
}

void* aligned_alloc(size_t alignment, size_t size) {
    void* ptr = SK_ALLOCATOR(aligned_alloc)(alignment, size);

    sk_mem_stat_t* stat = _get_stat();
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->naligned_alloc);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(ptr));
    }

    return ptr;
}

