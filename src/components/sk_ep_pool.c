#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "flibs/fhash.h"
#include "flibs/flist.h"
#include "flibs/fev.h"
#include "flibs/fev_buff.h"
#include "flibs/fnet.h"
#include "flibs/ftime.h"

#include "api/sk_const.h"
#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_entity.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_entity_util.h"
#include "api/sk_timer_service.h"
#include "api/sk_ep_pool.h"

typedef enum sk_ep_st_t {
    SK_EP_ST_INIT       = 0,
    SK_EP_ST_CONNECTING = 1,
    SK_EP_ST_CONNECTED  = 2,
    SK_EP_ST_SENT       = 3,
    SK_EP_ST_ERROR      = 4,
    SK_EP_ST_TIMEOUT    = 5
} sk_ep_st_t;

struct sk_ep_t;
typedef struct sk_ep_conn_t {
    sk_entity_t*    timer_entity;
    sk_timer_t*     timer;
    struct sk_ep_t* ep;
} sk_ep_conn_t;

struct sk_ep_mgr_t;
typedef struct sk_ep_data_t {
    sk_ep_handler_t    handler;
    sk_ep_cb_t         cb;
    void*              ud;
    unsigned long long start;
    const void*        data;
    size_t             count;
    const sk_entity_t* e;
} sk_ep_data_t;

typedef struct sk_ep_t {
    uint64_t     ekey;
    char         ipkey[SK_EP_KEY_MAX];
    sk_ep_type_t type;
    sk_ep_st_t   status;
    sk_ep_conn_t conn;
    struct sk_ep_mgr_t* owner;
    sk_entity_t* ep;
    sk_ep_data_t curr;
} sk_ep_t;

struct sk_ep_pool_t;
typedef struct sk_ep_mgr_t {
    struct sk_ep_pool_t* owner;
    sk_entity_mgr_t* eps;

    // mapping
    fhash* ee;   // entity to (ip.port <--> ep) mapping
    flist* tmp;

    int    max;
    int    nep;
} sk_ep_mgr_t;

struct sk_ep_pool_t {
    // SK_EP_TCP
    sk_ep_mgr_t*   tcp;

    // evlp
    fev_state*     evlp;

    // timer service
    sk_timersvc_t* tmsvc;
};

static
void _recv_timeout(sk_entity_t* entity, int valid, sk_obj_t* ud);

static
void sk_ep_mgr_del(sk_ep_mgr_t* mgr, sk_ep_t* ep);

static
void sk_ep_destroy(sk_ep_t* ep)
{
    if (!ep) return;

    // release user resources
    if (ep->curr.handler.release) {
        ep->curr.handler.release(ep->curr.ud);
    }

    // reset all other fields
    memset(&ep->conn, 0, sizeof(ep->conn));
    memset(&ep->curr, 0, sizeof(ep->curr));

    free(ep);
}

static
sk_ep_mgr_t* sk_ep_mgr_create(sk_ep_pool_t* owner, int max)
{
    if (max <= 0) return NULL;

    sk_ep_mgr_t* mgr = calloc(1, sizeof(*mgr));
    mgr->owner = owner;
    mgr->eps   = sk_entity_mgr_create(0);
    mgr->ee    = fhash_u64_create(0, FHASH_MASK_AUTO_REHASH);
    mgr->tmp   = flist_create();
    mgr->max   = max;

    return mgr;
}

static
void sk_ep_mgr_destroy(sk_ep_mgr_t* mgr)
{
    if (!mgr) return;

    sk_entity_mgr_destroy(mgr->eps);

    // Destroy mapping
    fhash_u64_iter eiter = fhash_u64_iter_new(mgr->ee);
    fhash* ipm = NULL;
    while ((ipm = fhash_u64_next(&eiter))) {
        fhash_str_iter ipiter = fhash_str_iter_new(ipm);
        flist* ep_list = NULL;
        while ((ep_list = fhash_str_next(&ipiter))) {
            sk_ep_t* ep = NULL;
            while ((ep = flist_pop(ep_list))) {
                sk_print("destroy ep: %p\n", (void*)ep);
                sk_ep_destroy(ep);
            }
            flist_delete(ep_list);
        }
        fhash_str_iter_release(&ipiter);
        fhash_str_delete(ipm);
    }
    fhash_u64_iter_release(&eiter);
    fhash_u64_delete(mgr->ee);

    flist_delete(mgr->tmp);
    free(mgr);
}

// TODO: optimize this part of performance
static
void _remove_ep(sk_ep_mgr_t* mgr, flist* ep_list, sk_ep_t* ep)
{
    if (!ep_list) return;
    if (flist_empty(ep_list)) return;

    sk_print("_remove_ep\n");
    sk_ep_t* t = NULL;
    while ((t = flist_pop(ep_list))) {
        sk_print("_remove_ep: t: %p, ep: %p\n", (void*)t, (void*)ep);
        if (t != ep) {
            sk_print("  _remove_ep: move t :%p to tmp\n", (void*)t);
            flist_push(mgr->tmp, t);
            continue;
        }
    }

    while ((t = flist_pop(mgr->tmp))) {
        flist_push(ep_list, t);
    }

    SK_ASSERT(flist_empty(mgr->tmp));
}

static
sk_ep_t* _find_ep(sk_ep_mgr_t* mgr, flist* ep_list,
                  const sk_ep_handler_t* handler)
{
    if (!ep_list) return NULL;
    if (flist_empty(ep_list)) return NULL;

    sk_ep_t* t = NULL;
    flist_iter iter = flist_new_iter(ep_list);
    while ((t = flist_each(&iter))) {
        if (t->status == SK_EP_ST_CONNECTED) {
            return t;
        }
    }

    return NULL;
}

static
void sk_ep_mgr_del(sk_ep_mgr_t* mgr, sk_ep_t* ep)
{
    if (!ep) return;

    sk_entity_mgr_t* emgr = sk_entity_owner(ep->ep);
    SK_ASSERT(emgr);
    SK_ASSERT(ep->owner == mgr);

    // 1. update mapping
    sk_print("sk_ep_mgr_del: ekey: %" PRId64 "\n", ep->ekey);
    fhash* ipm = fhash_u64_get(mgr->ee, ep->ekey);
    if (ipm) {
        sk_print("sk_ep_mgr_del: ipkey: %s\n", ep->ipkey);
        flist* ep_list = fhash_str_get(ipm, ep->ipkey);
        if (ep_list) {
            sk_print("sk_ep_mgr_del: remove it from ep_list\n");
            _remove_ep(mgr, ep_list, ep);
        }
    }

    // 2. clean up ep entity
    mgr->nep--;
    sk_entity_mgr_del(emgr, ep->ep);
    sk_entity_mgr_clean_dead(emgr);
    ep->ep = NULL;

    // 3. destroy ep
    sk_ep_destroy(ep);
}

static
void _timer_data_destroy(sk_ud_t ud)
{
    sk_print("ep connecting timer data destroying...\n");
    sk_ep_conn_t* conn = ud.ud;
    sk_entity_t* timer_entity = conn->timer_entity;

    // Clean the entity if it's still no owner
    if (NULL == sk_entity_owner(timer_entity)) {
        sk_entity_safe_destroy(timer_entity);
    }

    free(conn);
}

static
void _ep_send(sk_ep_t* ep, const void* data, size_t len)
{
    if (ep->status != SK_EP_ST_CONNECTED) {
        // ep still not ready (under connecting), just return
        sk_print("ep still not ready (under connecting), just return\n");
        return;
    }

    sk_entity_write(ep->ep, data, len);

    // If the ep is concurrent, which means it shouldn't be pended on response
    if (ep->curr.handler.flags & SK_EP_F_CONCURRENT) {
        return;
    }

    ep->status = SK_EP_ST_SENT;

    // Set up a timeout timer if needed
    if (ep->curr.handler.timeout && ep->curr.cb && ep->curr.handler.unpack) {
        unsigned long long consumed = ftime_gettime() - ep->curr.start;
        sk_ep_mgr_t* mgr = ep->owner;

        sk_ep_conn_t* conn = calloc(1, sizeof(*conn));
        conn->timer_entity = sk_entity_create(NULL);

        sk_ud_t cb_data  = {.ud = conn};
        sk_obj_opt_t opt = {.preset = NULL, .destroy = _timer_data_destroy};
        sk_obj_t* param_obj = sk_obj_create(opt, cb_data);

        conn->timer =
            sk_timersvc_timer_create(mgr->owner->tmsvc, ep->conn.timer_entity,
                (uint32_t)(ep->curr.handler.timeout - consumed), _recv_timeout,
                param_obj);
        SK_ASSERT(conn->timer);

        conn->ep = ep;
        ep->conn = *conn;
    }
}

static
void _handle_timeout(sk_ep_t* ep, int latency)
{
    if (ep->status != SK_EP_ST_CONNECTING &&
        ep->status != SK_EP_ST_SENT) {
        sk_print("ep is not in connecting/sent, skip it\n");
        return;
    }

    ep->status = SK_EP_ST_TIMEOUT;
    sk_ep_ret_t ret = {SK_EP_TIMEOUT, latency};
    ep->curr.cb(ret, NULL, 0, ep->curr.ud);

    sk_ep_mgr_del(ep->owner, ep);
}

static
void _handle_error(sk_ep_t* ep, int latency)
{
    sk_print("handle_error, ep: %p, latency: %d\n", (void*)ep, latency);
    ep->status = SK_EP_ST_ERROR;
    sk_ep_ret_t ret = {SK_EP_ERROR, latency};
    ep->curr.cb(ret, NULL, 0, ep->curr.ud);

    if (ep->conn.timer) {
        sk_timer_cancel(ep->conn.timer);
        ep->conn.timer = NULL;
    }

    sk_ep_mgr_del(ep->owner, ep);
}

static
void _handle_send(sk_ep_t* ep)
{
    _ep_send(ep, ep->curr.data, ep->curr.count);
}

static
void _recv_timeout(sk_entity_t* entity, int valid, sk_obj_t* ud)
{
    if (!valid) {
        sk_print("recv_timeout: timer invalid, skip it\n");
        return;
    }

    sk_ep_conn_t* conn = sk_obj_get(ud).ud;
    sk_ep_t* ep = conn->ep;

    int latency = (int)(ftime_gettime() - ep->curr.start);
    _handle_timeout(ep, latency);
}

static
void _read_cb(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_ep_t* ep = arg;
    SK_ASSERT(ep->status == SK_EP_ST_CONNECTED || ep->status == SK_EP_ST_SENT);

    size_t read_len = 0;
    size_t used_len = fevbuff_get_usedlen(evbuff, FEVBUFF_TYPE_READ);
    if (used_len == 0) {
        read_len = 1024;
    } else {
        read_len = used_len * 2;
    }

    ssize_t bytes = sk_entity_read(ep->ep, NULL, read_len);
    sk_print("entity read bytes: %ld\n", bytes);
    if (bytes <= 0) {
        sk_print("entity buffer cannot read\n");
        return;
    }

    const void* data = fevbuff_rawget(evbuff);
    size_t consumed = ep->curr.handler.unpack(ep->curr.ud, data, (size_t)bytes);
    if (consumed == 0) {
        // means user need more data, re-try in next round
        sk_print("user need more data, current data size=%zu\n", bytes);
        return;
    }

    sk_ep_ret_t ret = {SK_EP_OK, (int) (ftime_gettime() - ep->curr.start)};
    ep->curr.cb(ret, data, consumed, ep->curr.ud);

    fevbuff_pop(evbuff, consumed);

    // Set status to connected, so that others can pick this ep again
    if (!(ep->curr.handler.flags & SK_EP_F_CONCURRENT)) {
        ep->status = SK_EP_ST_CONNECTED;
    }

    // cancel the timer if needed
    if (ep->conn.timer) {
        sk_timer_cancel(ep->conn.timer);
    }
}

static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_ep_t* ep = arg;
    _handle_error(ep, (int)(ftime_gettime() - ep->curr.start));
}

static
void _on_connect(fev_state* fev, int fd, int mask, void* arg)
{
    sk_ep_t* ep = arg;

    int sockfd = fd;
    fev_del_event(fev, sockfd, FEV_READ | FEV_WRITE);

    sk_timer_cancel(ep->conn.timer);
    ep->conn.timer = NULL;

    if (mask & FEV_ERROR) {
        goto CONN_ERROR;
    }

    if (ep->status != SK_EP_ST_CONNECTING) {
        SK_ASSERT(0);
        sk_ep_mgr_del(ep->owner, ep);
        return;
    }

    int err = 0;
    socklen_t len = sizeof(int);
    if ((0 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len))) {
        if (0 == err) {
            sk_ep_mgr_t* mgr = ep->owner;
            fev_buff* evbuff =
                fevbuff_new(mgr->owner->evlp, sockfd, _read_cb, _error, ep);
            SK_ASSERT(evbuff);
            sk_entity_net_create(ep->ep, evbuff);
            ep->status = SK_EP_ST_CONNECTED;

            _handle_send(ep);
            return;
        }
    }

CONN_ERROR:
    _handle_error(ep, (int)(ftime_gettime() - ep->curr.start));
}

static
void _conn_timeout(sk_entity_t* entity, int valid, sk_obj_t* ud)
{
    if (!valid) {
        sk_print("connection timer not valid, skip it\n");
        return;
    }

    sk_ep_conn_t* conn = sk_obj_get(ud).ud;
    sk_ep_t* ep = conn->ep;
    if (ep->status != SK_EP_ST_CONNECTING) {
        sk_print("ep is not in connecting, skip it, status=%d\n", ep->status);
        return;
    }

    int latency = (int)(ftime_gettime() - ep->curr.start);
    _handle_timeout(ep, latency);
}

// Currently only support ipv4
static
int _create_entity_tcp(sk_ep_mgr_t* mgr, const sk_ep_handler_t* handler,
                       sk_ep_t* ep, unsigned long long start)
{
    unsigned long long now = ftime_gettime();
    unsigned long long consumed = now - start;
    ep->status = SK_EP_ST_CONNECTING;

    int sockfd = -1;
    int s = fnet_conn_async(handler->ip, handler->port, &sockfd);
    if (s == 0) {
        sk_entity_t* net_entity = ep->ep;

        fev_buff* evbuff =
            fevbuff_new(mgr->owner->evlp, sockfd, _read_cb, _error, ep);
        SK_ASSERT(evbuff);
        sk_entity_net_create(net_entity, evbuff);

        ep->status = SK_EP_ST_CONNECTED;
        return 0;
    } else if (s > 0) {
        sk_ep_conn_t* conn = calloc(1, sizeof(*conn));
        conn->timer_entity = sk_entity_create(NULL);

        sk_ud_t cb_data  = {.ud = conn};
        sk_obj_opt_t opt = {.preset = NULL, .destroy = _timer_data_destroy};
        sk_obj_t* param_obj = sk_obj_create(opt, cb_data);

        conn->timer =
            sk_timersvc_timer_create(mgr->owner->tmsvc, conn->timer_entity,
                (uint32_t)(handler->timeout - consumed), _conn_timeout,
                param_obj);
        SK_ASSERT(conn->timer);

        int ret = fev_reg_event(mgr->owner->evlp, sockfd, FEV_WRITE, NULL,
                                _on_connect, ep);
        SK_ASSERT(!ret);

        conn->ep = ep;
        ep->conn = *conn;
        return 0;
    } else {
        return -1;
    }
}

static
sk_ep_t* _ep_create(sk_ep_mgr_t* mgr, const sk_ep_handler_t* handler,
                    uint64_t ekey, const char* ipkey,
                    unsigned long long start)
{
    sk_ep_t* ep = calloc(1, sizeof(*ep));
    ep->ekey   = ekey;
    strncpy(ep->ipkey, ipkey, SK_EP_KEY_MAX);
    ep->type   = handler->type;
    ep->status = SK_EP_ST_INIT;
    ep->ep     = sk_entity_create(NULL);
    SK_ASSERT(ep->ep);
    ep->owner  = mgr;

    return ep;
}

static
sk_ep_t* _ep_mgr_get_or_create(sk_ep_mgr_t*           mgr,
                               const sk_entity_t*     entity,
                               const sk_ep_handler_t* handler,
                               unsigned long long     start,
                               sk_ep_cb_t             cb,
                               void*                  ud,
                               const void*            data,
                               size_t                 count)
{
    fhash*   ipm     = NULL;
    flist*   ep_list = NULL;
    sk_ep_t* ep      = NULL;
    uint64_t ekey    = 0;

    int flags = handler->flags;
    if (flags & SK_EP_F_ORPHAN) {
        ekey = 0;
    } else {
        ekey = (uint64_t) (uintptr_t) entity;
    }

    ipm = fhash_u64_get(mgr->ee, ekey);
    if (!ipm) {
        ipm = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
        fhash_u64_set(mgr->ee, ekey, ipm);
    }

    char ipkey[SK_EP_KEY_MAX];
    snprintf(ipkey, SK_EP_KEY_MAX, "%s.%d", handler->ip, handler->port);
    ep_list = fhash_str_get(ipm, ipkey);
    if (!ep_list) {
        ep_list = flist_create();
        fhash_str_set(ipm, ipkey, ep_list);
    }

    ep = _find_ep(mgr, ep_list, handler);
    if (!ep) {
        ep = _ep_create(mgr, handler, ekey, ipkey, start);
        if (!ep) return NULL;

        sk_entity_mgr_add(mgr->eps, ep->ep);
        flist_push(ep_list, ep);
        mgr->nep++;
    }

    // Update ep status
    sk_ep_data_t curr_data = {
        .handler = *handler,
        .cb      = cb,
        .ud      = ud,
        .start   = start,
        .data    = data,
        .count   = count,
        .e       = entity
    };

    ep->curr = curr_data;

    if (ep->status == SK_EP_ST_INIT) {
        int r = _create_entity_tcp(mgr, handler, ep, start);
        if (r) {
            sk_ep_mgr_del(mgr, ep);
            return NULL;
        }
    }

    return ep;
}

// Public APIs
sk_ep_pool_t* sk_ep_pool_create(void* evlp, sk_timersvc_t* tmsvc, int max)
{
    sk_ep_pool_t* pool = calloc(1, sizeof(*pool));
    pool->tcp   = sk_ep_mgr_create(pool, max);
    pool->evlp  = evlp;
    pool->tmsvc = tmsvc;

    return pool;
}

void sk_ep_pool_destroy(sk_ep_pool_t* pool)
{
    if (!pool) return;

    sk_ep_mgr_destroy(pool->tcp);
    free(pool);
}

int sk_ep_send(sk_ep_pool_t* pool, const sk_entity_t* entity,
               const sk_ep_handler_t handler,
               const void* data, size_t count,
               sk_ep_cb_t cb, void* ud)
{
    SK_ASSERT(pool);
    unsigned long long start = ftime_gettime();

    // 1. check
    if (!data || !count) {
        return 1;
    }

    if (!handler.ip || handler.port == 0) {
        return 1;
    }

    if (handler.unpack && handler.timeout == 0) {
        return 1;
    }

    // 2. get ep mgr
    sk_ep_mgr_t* mgr = NULL;
    sk_ep_type_t et = handler.type;
    if (et == SK_EP_TCP) {
        mgr = pool->tcp;
    } else {
        return 1;
    }

    // 3. pick up one ep from ep mgr
    if (mgr->nep == mgr->max) {
        return 1;
    }

    sk_ep_t* ep =
        _ep_mgr_get_or_create(mgr, entity, &handler, start, cb, ud, data, count);
    if (!ep) {
        return 1;
    }

    // 4. send via ep
    _ep_send(ep, data, count);
    return 0;
}
