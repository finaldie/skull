#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <pthread.h>
#include <malloc.h>

#include "flibs/compiler.h"
#include "flibs/fhash.h"
#include "flibs/fmbuf.h"
#include "api/sk_env.h"
#include "api/sk_time.h"
#include "api/sk_const.h"
#include "api/sk_utils.h"
#include "api/sk_core.h"
#include "api/sk_service.h"
#include "api/sk_malloc.h"
#include "api/sk_log.h"

#define SK_MEMDUMP_MAX_LENGTH  (8192)
#define SK_MEMDUMP_LINE_LENGTH (1024)

// Global Memory Tracer Tag
#define SK_GMTT "[MEM_TRACE]"
#define SK_FILL_TAGS(tn, c, n) \
{ \
    *tname = tn; \
    *component = c; \
    *name = n; \
}

#define SK_MT_HEADER_FMT "%lu.%lu %s %s:%s.%s - "
#define _SK_MT_ALLOC_FMT(symbol) #symbol " %p %zu bytes %lu ns from %p"
#define _SK_MT_REALLOC_FMT "realloc %p-%p %zu-%zu bytes %lu ns from %p %s"

#define SK_MT_FMT(symbol) SK_MT_HEADER_FMT _SK_MT_ALLOC_FMT(symbol)
#define SK_MT_FMT_REALLOC SK_MT_HEADER_FMT _SK_MT_REALLOC_FMT

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

#define SK_REAL_TYPE(symbol)  sk_real_ ## symbol ## _t
#define SK_REAL(symbol)      _sk_real_ ## symbol

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

/**
 * Re-entrance control
 */
#define SK_MEM_ENTER() \
    do { \
        bool ____ready = sk_thread_env_ready(); \
        sk_thread_env_t* ____env  = SK_ENV; \
        sk_mem_env_t*    ____menv = _mem_env(); \
        int* ____api_entrance = (____ready && ____env && ____menv) \
            ? &____menv->api_entrance : &imem.api_entrance; \
        (*____api_entrance)++; \
        do { \

#define SK_MEM_CHECK_AND_BREAK() \
            if (unlikely(*____api_entrance > 1)) break;

#define SK_MEM_EXIT() \
        } while(0); \
        (*____api_entrance)--; \
    } while(0)

// Thread-local data format
// TODO: All these thread-local data should use `_Thread_local` keyword in C11
typedef struct sk_mem_env_t {
    int api_entrance;
} sk_mem_env_t;

/**
 * The memory structure is divided to two parts majorly: init and runtime parts.
 * In the runtime part, it also can be divided to 3 parts:
 *  - core
 *  - module
 *  - service
 */
typedef struct sk_mem_t {
    bool          tracing_enabled;

    char          _padding[sizeof(void*) - sizeof(bool)];

    // The mem_stat before thread_env been set up (mainly for counting the stat
    //  during the initialization)
    sk_mem_stat_t init;
    int           api_entrance;

    pthread_key_t key;

    // Core mem_stat
    // notes: the api_entrance counter is in thread_local
    sk_mem_stat_t core;

    // stat hashmap for storing the stat for modules
    // key: module name. value: stat
    // notes: the api_entrance counter is in thread_local
    fhash*        mstats;

    // stat hashmap for storing the stat for services
    // key: service name. value: stat
    // notes: the api_entrance counter is in thread_local
    fhash*        sstats;

    sk_logger_t*  logger;
} sk_mem_t;

static sk_mem_t imem;

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
sk_mem_env_t* _mem_env() {
    sk_mem_env_t* env = pthread_getspecific(imem.key);
    if (!env) {
        env = SK_ALLOCATOR(calloc)(1, sizeof(sk_mem_env_t));
        pthread_setspecific(imem.key, env);
    }

    return env;
}

static
sk_mem_stat_t* _get_stat(const char** tname,
                         const char** component,
                         const char** name) {
    sk_mem_stat_t* stat = NULL;

    if (unlikely(!(sk_thread_env_ready() && SK_ENV && _mem_env()))) {
        stat = &imem.init;
        SK_FILL_TAGS("main", "skull", "init");
    } else {
        sk_env_pos_t pos = SK_ENV_POS;

        switch (pos) {
        case SK_ENV_POS_MODULE: {
            const char* mname = ((sk_module_t*)(SK_ENV_CURRENT))->cfg->name;
            stat = (sk_mem_stat_t*)sk_mem_stat_module(mname);
            SK_FILL_TAGS(SK_ENV_NAME, "module", mname);
            break;
        }
        case SK_ENV_POS_SERVICE: {
            const char* sname = sk_service_name(SK_ENV_CURRENT);
            stat = (sk_mem_stat_t*)sk_mem_stat_service(sname);
            SK_FILL_TAGS(SK_ENV_NAME, "service", sname);
            break;
        }
        case SK_ENV_POS_CORE:
        default:
            stat = &imem.core;
            SK_FILL_TAGS(SK_ENV_NAME, "skull", "core");
            break;
        }
    }

    return stat;
}

static
void _stat_tbl_destroy(fhash* tbl) {
    if (!tbl) return;

    sk_mem_stat_t* stat = NULL;
    fhash_str_iter iter = fhash_str_iter_new(tbl);
    while ((stat = fhash_str_next(&iter)) != NULL) {
        free(stat);
    }

    fhash_str_iter_release(&iter);
    fhash_str_delete(tbl);
}

static
void _stats_dump_to_string(fhash* tbl, fmbuf* logbuf) {
    char line[SK_MEMDUMP_LINE_LENGTH];

    fhash_str_iter iter = fhash_str_iter_new(tbl);
    sk_mem_stat_t* stat = NULL;

    while ((stat = fhash_str_next(&iter)) != NULL) {
        int len = snprintf(line, SK_MEMDUMP_LINE_LENGTH,
                           " %s:%zu",
                           iter.key,
                           sk_mem_allocated(stat));

        fmbuf_push(logbuf, line, (size_t)len);
    }

    fhash_str_iter_release(&iter);
}

static
void _thread_exit(void* data) {
    sk_print("[mem] thread exit:\n");
    SK_ALLOCATOR(free)(data);
}

/**
 * Public APIs
 */
void sk_mem_init(const char* workdir,
                 const char* logname,
                 int  log_level,
                 bool stdout_fwd) {
    pthread_key_create(&imem.key, _thread_exit);

    imem.mstats = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    imem.sstats = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    imem.logger = sk_logger_create(workdir, logname, log_level,
                                   false, stdout_fwd);
}

void sk_mem_destroy() {
    sk_mem_dump("DESTROY");

    imem.tracing_enabled = false;
    pthread_key_delete(imem.key);
    imem.key = 0;

    _stat_tbl_destroy(imem.mstats);
    imem.mstats = NULL;

    _stat_tbl_destroy(imem.sstats);
    imem.sstats = NULL;

    sk_logger_destroy(imem.logger);
}

const sk_mem_stat_t* sk_mem_stat_static() {
    return &imem.init;
}

const sk_mem_stat_t* sk_mem_stat_core() {
    return &imem.core;
}

size_t sk_mem_allocated(const sk_mem_stat_t* stat) {
    volatile size_t alloc_sz  = stat->alloc_sz;
    volatile size_t dalloc_sz = stat->dalloc_sz;

    return alloc_sz > dalloc_sz ? alloc_sz - dalloc_sz : 0;
}

void sk_mem_stat_module_register(const char* name) {
    fhash_str_set(imem.mstats, name, calloc(1, sizeof(sk_mem_stat_t)));
}

void sk_mem_stat_service_register(const char* name) {
    fhash_str_set(imem.sstats, name, calloc(1, sizeof(sk_mem_stat_t)));
}

const sk_mem_stat_t* sk_mem_stat_module (const char* name) {
    return fhash_str_get(imem.mstats, name);
}

const sk_mem_stat_t* sk_mem_stat_service(const char* name) {
    return fhash_str_get(imem.sstats, name);
}

bool sk_mem_trace_status() {
    return imem.tracing_enabled && imem.logger;
}

void sk_mem_trace(bool enabled) {
    imem.tracing_enabled = enabled;
}

void sk_mem_dump(const char* fmt, ...) {
    char tag[SK_MEMDUMP_LINE_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsnprintf(tag, SK_MEMDUMP_LINE_LENGTH, fmt, args);
    va_end(args);

    fmbuf* mlogbuf = fmbuf_create(SK_MEMDUMP_MAX_LENGTH);
    _stats_dump_to_string(imem.mstats, mlogbuf);

    fmbuf* slogbuf = fmbuf_create(SK_MEMDUMP_MAX_LENGTH);
    _stats_dump_to_string(imem.sstats, slogbuf);

    SK_LOG_INFO(imem.logger, "[MEM_DUMP %s] Init:%zu; Core:%zu; "
                "Modules:%s; Services:%s",
                tag,
                sk_mem_allocated(&imem.init),
                sk_mem_allocated(&imem.core),
                fmbuf_head(mlogbuf), fmbuf_head(slogbuf));

    fmbuf_delete(mlogbuf);
    fmbuf_delete(slogbuf);
}

/**
 * Override libc, to measure module/service memory usage
 */
void* malloc(size_t sz) {
    void* ptr = NULL;
    SK_MEM_ENTER();

    ulong_t start = sk_gettime();
    ptr = SK_ALLOCATOR(malloc)(sz);
    ulong_t duration = sk_gettime() - start;

    const char* tname = NULL, *comp = NULL, *name = NULL;
    sk_mem_stat_t* stat = _get_stat(&tname, &comp, &name);
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->nmalloc);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(ptr));
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    SK_MEM_CHECK_AND_BREAK();
    if (unlikely(sk_mem_trace_status())) {
        SK_LOG_INFO(imem.logger, SK_MT_FMT(malloc),
                start / 1000000000l, start % 1000000000l,
                SK_GMTT, tname, comp, name, ptr, sz,
                duration, __builtin_return_address(0));
    }

    SK_MEM_EXIT();
    return ptr;
}

void free(void* ptr) {
    SK_MEM_ENTER();
    const char* tname = NULL, *comp = NULL, *name = NULL;
    sk_mem_stat_t* stat = _get_stat(&tname, &comp, &name);
    size_t sz = _get_malloc_sz(ptr);
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->nfree);
        SK_ATOMIC_ADD(stat->dalloc_sz, sz);
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    ulong_t start = sk_gettime();
    SK_ALLOCATOR(free)(ptr);
    ulong_t duration = sk_gettime() - start;

    SK_MEM_CHECK_AND_BREAK();
    if (unlikely(sk_mem_trace_status())) {
        SK_LOG_INFO(imem.logger, SK_MT_FMT(free),
                start / 1000000000l, start % 1000000000l,
                SK_GMTT, tname, comp, name, ptr, sz,
                duration, __builtin_return_address(0));
    }

    SK_MEM_EXIT();
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
    void* ptr = NULL;
    SK_MEM_ENTER();

    ulong_t start = sk_gettime();
    ptr = SK_ALLOCATOR(calloc)(nmemb, sz);
    ulong_t duration = sk_gettime() - start;

    const char* tname = NULL, *comp = NULL, *name = NULL;
    sk_mem_stat_t* stat = _get_stat(&tname, &comp, &name);
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->ncalloc);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(ptr));
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    SK_MEM_CHECK_AND_BREAK();
    if (unlikely(sk_mem_trace_status())) {
        SK_LOG_INFO(imem.logger, SK_MT_FMT(calloc),
                start / 1000000000l, start % 1000000000l,
                SK_GMTT, tname, comp, name, ptr, _get_malloc_sz(ptr),
                duration, __builtin_return_address(0));
    }

    SK_MEM_EXIT();
    return ptr;
}

void* realloc(void* ptr, size_t sz) {
    void* nptr = NULL;
    SK_MEM_ENTER();

    size_t old_sz = _get_malloc_sz(ptr);

    ulong_t start = sk_gettime();
    nptr = SK_ALLOCATOR(realloc)(ptr, sz);
    ulong_t duration = sk_gettime() - start;

    size_t new_sz = _get_malloc_sz(nptr);
    const char* tname = NULL, *comp = NULL, *name = NULL;
    sk_mem_stat_t* stat = _get_stat(&tname, &comp, &name);

    const char* warning = "";
    if (unlikely(!ptr && sz == 0)) {
        warning = "Possible leaked or error";
    }

    if (likely(stat)) {
        SK_ATOMIC_INC(stat->nrealloc);

        if (unlikely(!ptr)) {
            SK_ATOMIC_ADD(stat->alloc_sz, new_sz);
        } else if (sz == 0) {
            SK_ATOMIC_ADD(stat->dalloc_sz, old_sz);
        } else if (old_sz != new_sz) {
            SK_ATOMIC_ADD(stat->alloc_sz, new_sz);
            SK_ATOMIC_ADD(stat->dalloc_sz, old_sz);
        }

        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    SK_MEM_CHECK_AND_BREAK();
    if (unlikely(sk_mem_trace_status())) {
        SK_LOG_INFO(imem.logger, SK_MT_FMT_REALLOC,
                start / 1000000000l, start % 1000000000l,
                SK_GMTT, tname, comp, name, ptr, nptr, old_sz, new_sz,
                duration, __builtin_return_address(0), warning);
    }

    SK_MEM_EXIT();
    return nptr;
}

int posix_memalign(void **memptr, size_t alignment, size_t size) {
    int ret = 0;
    SK_MEM_ENTER();

    ulong_t start = sk_gettime();
    ret = SK_ALLOCATOR(posix_memalign)(memptr, alignment, size);
    ulong_t duration = sk_gettime() - start;

    const char* tname = NULL, *comp = NULL, *name = NULL;
    sk_mem_stat_t* stat = _get_stat(&tname, &comp, &name);
    if (likely(!ret && stat)) {
        SK_ATOMIC_INC(stat->nposix_memalign);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(*memptr));
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    SK_MEM_CHECK_AND_BREAK();
    if (unlikely(sk_mem_trace_status())) {
        SK_LOG_INFO(imem.logger, SK_MT_FMT(posix_memalign),
                start / 1000000000l, start % 1000000000l,
                SK_GMTT, tname, comp, name, *memptr, _get_malloc_sz(*memptr),
                duration, __builtin_return_address(0));
    }

    SK_MEM_EXIT();
    return ret;
}

void* aligned_alloc(size_t alignment, size_t size) {
    void* ptr = NULL;
    SK_MEM_ENTER();

    ulong_t start = sk_gettime();
    ptr = SK_ALLOCATOR(aligned_alloc)(alignment, size);
    ulong_t duration = sk_gettime() - start;

    const char* tname = NULL, *comp = NULL, *name = NULL;
    sk_mem_stat_t* stat = _get_stat(&tname, &comp, &name);
    if (likely(stat)) {
        SK_ATOMIC_INC(stat->naligned_alloc);
        SK_ATOMIC_ADD(stat->alloc_sz, _get_malloc_sz(ptr));
        SK_ATOMIC_UPDATE_PEAKSZ(stat);
    }

    SK_MEM_CHECK_AND_BREAK();
    if (unlikely(sk_mem_trace_status())) {
        SK_LOG_INFO(imem.logger, SK_MT_FMT(aligned_alloc),
                start / 1000000000l, start % 1000000000l,
                SK_GMTT, tname, comp, name, ptr, _get_malloc_sz(ptr),
                duration, __builtin_return_address(0));
    }

    SK_MEM_EXIT();
    return ptr;
}

