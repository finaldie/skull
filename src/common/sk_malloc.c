#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>

#include "flibs/compiler.h"

#ifdef SKULL_JEMALLOC_LINKED
# include "jemalloc/jemalloc.h"
# define SK_ALLOCATOR(symbol) je_ ## symbol
#else
# define SK_ALLOCATOR(symbol) sk_real_ ## symbol
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

/**
 * Override libc
 */
void* malloc(size_t sz) {
    return SK_ALLOCATOR(malloc)(sz);
}

void free(void* ptr) {
    SK_ALLOCATOR(free)(ptr);
}

void* calloc(size_t nmemb, size_t sz) {
    return SK_ALLOCATOR(calloc)(nmemb, sz);
}

void* realloc(void* ptr, size_t sz) {
    return SK_ALLOCATOR(realloc)(ptr, sz);
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    return SK_ALLOCATOR(posix_memalign)(memptr, alignment, size);
}

void* aligned_alloc(size_t alignment, size_t size) {
    return SK_ALLOCATOR(aligned_alloc)(alignment, size);
}

