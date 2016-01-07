#include <stdlib.h>
#include <stdint.h>

#include "flibs/fhash.h"
#include "api/sk_entity.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_entity_util.h"
#include "api/sk_ep_pool.h"

typedef enum sk_ep_st_t {
    SK_EP_ST_INIT      = 0,
    SK_EP_ST_CONNECTED = 1,
    SK_EP_ST_ERROR     = 2
} sk_ep_st_t;

typedef struct sk_ep_t {
    sk_ep_type_t type;
    sk_ep_st_t   status;
    uint32_t     inuse     :1;
    uint32_t     _reserved :31;

#if __WORDSIZE == 64
    int          _padding;
#endif

    sk_entity_t* ep;
} sk_ep_t;

typedef struct sk_ep_mgr_t {
    sk_entity_mgr_t* eps;
    fhash* used;
    fhash* free;

    int    max;

#if __WORDSIZE == 64
    int    _padding;
#endif
} sk_ep_mgr_t;

struct sk_ep_pool_t {
    // SK_EP_TCP
    sk_ep_mgr_t* tcp;
};

static
sk_ep_mgr_t* sk_ep_mgr_create(int max)
{
    if (max <= 0) return NULL;

    sk_ep_mgr_t* mgr = calloc(1, sizeof(*mgr));
    mgr->eps  = sk_entity_mgr_create(0);
    mgr->used = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    mgr->free = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    mgr->max = max;

    return mgr;
}

static
void sk_ep_mgr_destroy(sk_ep_mgr_t* mgr)
{
    if (!mgr) return;

    sk_entity_mgr_destroy(mgr->eps);
    fhash_str_delete(mgr->used);
    fhash_str_delete(mgr->free);
    free(mgr);
}

// Public APIs
sk_ep_pool_t* sk_ep_pool_create(int max)
{
    sk_ep_pool_t* pool = calloc(1, sizeof(*pool));
    pool->tcp = sk_ep_mgr_create(max);

    return pool;
}

void sk_ep_pool_destroy(sk_ep_pool_t* pool)
{
    if (!pool) return;

    sk_ep_mgr_destroy(pool->tcp);
    free(pool);
}

int sk_ep_send(sk_ep_pool_t* pool, const sk_ep_handler_t handler,
               const void* data, size_t count,
               sk_ep_cb_t cb, void* ud)
{
    // 1. check

    // 2. get ep mgr

    // 3. pick up one ep from ep mgr

    // 4. send via ep
    return 0;
}
