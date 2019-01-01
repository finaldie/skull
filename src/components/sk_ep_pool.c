#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "flibs/fhash.h"
#include "flibs/flist.h"
#include "flibs/fdlist.h"
#include "flibs/fev.h"
#include "flibs/fev_buff.h"
#include "flibs/fnet.h"

#include "api/sk_const.h"
#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_time.h"
#include "api/sk_log_helper.h"
#include "api/sk_metrics.h"
#include "api/sk_entity.h"
#include "api/sk_entity_mgr.h"
#include "api/sk_entity_util.h"
#include "api/sk_timer_service.h"
#include "api/sk_ep_pool.h"

#define SK_METRICS_EP_OK() { \
    sk_metrics_global.ep_ok.inc(1); \
    sk_metrics_worker.ep_ok.inc(1); \
}

#define SK_METRICS_EP_ERROR() { \
    sk_metrics_global.ep_error.inc(1); \
    sk_metrics_worker.ep_error.inc(1); \
}

#define SK_METRICS_EP_TIMEOUT() { \
    sk_metrics_global.ep_timeout.inc(1); \
    sk_metrics_worker.ep_timeout.inc(1); \
}

#define SK_METRICS_EP_CREATE() { \
    sk_metrics_global.ep_create.inc(1); \
    sk_metrics_worker.ep_create.inc(1); \
}

#define SK_METRICS_EP_DESTROY() { \
    sk_metrics_global.ep_destroy.inc(1); \
    sk_metrics_worker.ep_destroy.inc(1); \
}

#define SK_METRICS_EP_SEND() { \
    sk_metrics_global.ep_send.inc(1); \
    sk_metrics_worker.ep_send.inc(1); \
}

#define SK_METRICS_EP_RECV() { \
    sk_metrics_global.ep_recv.inc(1); \
    sk_metrics_worker.ep_recv.inc(1); \
}

typedef enum sk_ep_st_t {
    SK_EP_ST_INIT       = 0,
    SK_EP_ST_CONNECTING = 1,
    SK_EP_ST_CONNECTED  = 2
} sk_ep_st_t;

typedef enum sk_ep_nst_t {
    SK_EP_NST_INIT    = 0,
    SK_EP_NST_PENDING = 1, // pending on the connection be ready
    SK_EP_NST_SENT    = 2,
} sk_ep_nst_t;

// 32bit platform: 8  Bytes
// 64bit platform: 12 Bytes
#define EPDATA_INIT_SZ (sizeof(size_t) + sizeof(int))

struct sk_ep_t;
typedef struct sk_ep_timerdata_t {
    sk_entity_t*   timer_entity;

    union {
        fdlist_node_t*  ep_node;
        struct sk_ep_t* ep;
    } data;
} sk_ep_timerdata_t;

typedef struct sk_ep_data_t {
    struct sk_ep_t*     owner;
    const sk_service_t* service;
    sk_ep_handler_t     handler;
    sk_ep_cb_t          cb;
    void*               ud;
    slong_t             start;
    const sk_entity_t*  e;       // sk_txn's entity

    sk_timer_t*        recv_timer;

    size_t             count;   // size of data (bytes)
    sk_ep_nst_t        status;
    char               data[EPDATA_INIT_SZ]; // data for sending
} sk_ep_data_t;

typedef struct sk_ep_readarg_t {
    const void*   data;     // in
    size_t        len;      // in
    ssize_t       consumed; // out
} sk_ep_readarg_t;

struct sk_ep_mgr_t;
typedef struct sk_ep_t {
    uint64_t            ekey;
    char                ipkey[SK_EP_KEY_MAX];
    sk_ep_type_t        type;
    sk_ep_st_t          status;
    sk_entity_t*        entity;   // network entity
    fdlist*             txns;     // ep txns, type: sk_ep_data_t

    int                 ntxn;
    int                 flags;

    slong_t             start;
    sk_timer_t*         conn_timer;
    sk_timer_t*         shutdown_timer;
    struct sk_ep_mgr_t* owner;

    int                 fd;

    int _padding;
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

    // SK_EP_UDP
    sk_ep_mgr_t*   udp;

    // evlp
    fev_state*     evlp;

    // timer service
    sk_timersvc_t* tmsvc;
};

static
void _recv_timeout(sk_entity_t* entity, int valid, sk_obj_t* ud);

static
void _shutdown_ep(sk_entity_t* entity, int valid, sk_obj_t* ud);

static
void _ep_mgr_del(sk_ep_mgr_t* mgr, sk_ep_t* ep);

static
sk_timer_t* _ep_create_timer(sk_ep_t* ep, sk_timer_triggered timer_cb,
                             uint32_t timeout);

static
int  sk_ep_timeout(sk_ep_data_t* ep_data)
{
    sk_print("sk_ep_timeout: timeout value: %d\n", ep_data->handler.timeout);
    if (ep_data->handler.timeout == 0) {
        return 0;
    }

    slong_t consumed = sk_time_ms() - ep_data->start;
    sk_print("sk_ep_timeout: consumed: %lu, timeout value: %d\n",
             consumed, ep_data->handler.timeout);

    if (consumed >= ep_data->handler.timeout) {
        return 1;
    } else {
        return 0;
    }
}

static
slong_t sk_ep_time_consumed(sk_ep_t* ep)
{
    return sk_time_ms() - ep->start;
}

static
slong_t sk_ep_txn_time_consumed(sk_ep_data_t* ep_data)
{
    return sk_time_ms() - ep_data->start;
}

static
slong_t sk_ep_txn_time_left(sk_ep_data_t* ep_data)
{
    slong_t consumed = sk_time_ms() - ep_data->start;
    if (consumed >= ep_data->handler.timeout) {
        return 0;
    } else {
        return ep_data->handler.timeout - consumed;
    }
}

static
void _ep_create_shutdown_timer(sk_ep_t* ep, uint32_t expiration) {
    sk_timer_t* shutdown_timer = ep->shutdown_timer;
    if (shutdown_timer) {
        sk_print("refresh shutdown timer: fd: %d, %u ms\n", ep->fd, expiration);
        int ret = sk_timer_resetn(shutdown_timer, expiration);
        if (!ret) {
            return;
        }

        sk_print("sk_timer_resetn failed, due to the timer is on the fly\n");
        sk_timer_cancel(shutdown_timer);
        ep->shutdown_timer = NULL;
    }

    sk_print("create shutdown timer: fd: %d, %u ms\n", ep->fd, expiration);
    ep->shutdown_timer =
        _ep_create_timer(ep, _shutdown_ep, expiration);

    SK_ASSERT(ep->shutdown_timer);
}

static
void _ep_data_release(sk_ep_data_t* ep_data, int release_userdata)
{
    if (ep_data->recv_timer) {
        sk_timer_cancel(ep_data->recv_timer);
        ep_data->recv_timer = NULL;
    }

    if (release_userdata && ep_data->handler.release) {
        SK_LOG_SETCOOKIE("service.%s", sk_service_name(ep_data->service));
        SK_ENV_POS_SAVE(SK_ENV_POS_SERVICE, ep_data->service);

        ep_data->handler.release(ep_data->ud);

        SK_ENV_POS_RESTORE();
        SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    }

    free(ep_data);
}

static
void _ep_node_destroy(fdlist_node_t* ep_node, int release_userdata)
{
    if (!ep_node) return;

    sk_ep_data_t* ep_data = fdlist_get_nodedata(ep_node);
    sk_ep_t*      ep      = ep_data->owner;
    _ep_data_release(ep_data, release_userdata);

    fdlist_delete_node(ep_node);
    fdlist_destroy_node(ep_node);
    ep->ntxn--;
}

static
void _ep_destroy(sk_ep_t* ep)
{
    if (!ep) return;

    // Cancel connection timer
    if (ep->conn_timer) {
        sk_timer_cancel(ep->conn_timer);
        ep->conn_timer = NULL;
    }

    // Cancel shutdown timer
    if (ep->shutdown_timer) {
        sk_timer_cancel(ep->shutdown_timer);
        ep->shutdown_timer = NULL;
    }

    // release unfinished user resources
    fdlist_node_t* node = NULL;
    while ((node = fdlist_pop(ep->txns))) {
        _ep_node_destroy(node, 1);
    }
    fdlist_destroy(ep->txns);

    // if no ep->entity and ep->fd >= 0 and ep->status == SK_EP_ST_CONNECTING, then we need to close this fd
    if (!ep->entity && ep->fd >= 0 && ep->status == SK_EP_ST_CONNECTING) {
        sk_print("close connection which under connecting..., fd: %d, ntxns: %d\n", ep->fd, ep->ntxn);
        fev_del_event(ep->owner->owner->evlp, ep->fd, FEV_READ | FEV_WRITE);
        close(ep->fd);
    }

    free(ep);
    SK_METRICS_EP_DESTROY();
}

static
sk_ep_mgr_t* sk_ep_mgr_create(sk_ep_pool_t* owner, int max)
{
    if (max <= 0) return NULL;

    sk_ep_mgr_t* mgr = calloc(1, sizeof(*mgr));
    mgr->owner = owner;
    mgr->eps   = sk_entity_mgr_create(SK_EP_POOL_MIN);
    mgr->ee    = fhash_u64_create(SK_EP_POOL_MIN, FHASH_MASK_AUTO_REHASH);
    mgr->tmp   = flist_create();
    mgr->max   = max;

    return mgr;
}

static
void sk_ep_mgr_destroy(sk_ep_mgr_t* mgr)
{
    if (!mgr) return;

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
                _ep_destroy(ep);
            }
            flist_delete(ep_list);
        }
        fhash_str_iter_release(&ipiter);
        fhash_str_delete(ipm);
    }
    fhash_u64_iter_release(&eiter);
    fhash_u64_delete(mgr->ee);

    // destroy entity manager
    sk_entity_mgr_destroy(mgr->eps);
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
        if (handler->flags == t->flags) {
            if (t->flags & SK_EP_F_CONCURRENT) {
                return t;
            } else if (t->status == SK_EP_ST_CONNECTED) {
                SK_ASSERT(t->ntxn <= 1);

                if (fdlist_empty(t->txns)) {
                    return t;
                }
            }
        }
    }

    return NULL;
}

static
void _ep_mgr_del(sk_ep_mgr_t* mgr, sk_ep_t* ep)
{
    if (!ep) return;

    sk_entity_mgr_t* emgr = sk_entity_owner(ep->entity);

    SK_ASSERT(emgr);
    SK_ASSERT(ep->owner == mgr);

    // 2.1. update mapping
    fhash* ipm = fhash_u64_get(mgr->ee, ep->ekey);
    if (ipm) {
        flist* ep_list = fhash_str_get(ipm, ep->ipkey);
        if (ep_list) {
            _remove_ep(mgr, ep_list, ep);
        }
    }

    // 2.2 Destroy ep entity
    mgr->nep--;
    sk_entity_mgr_del(emgr, ep->entity);
    sk_entity_mgr_clean_dead(emgr);
    ep->entity = NULL;

    // 2.3 Destroy ep
    _ep_destroy(ep);
}

static
void _timer_data_destroy(sk_ud_t ud)
{
    sk_print("ep timer data destroying...\n");
    sk_ep_timerdata_t* timerdata = ud.ud;
    sk_entity_t* timer_entity = timerdata->timer_entity;

    // Clean the entity if it's still no owner
    if (timer_entity && NULL == sk_entity_owner(timer_entity)) {
        sk_print("destroy timer entity\n");
        sk_entity_safe_destroy(timer_entity);
        timerdata->timer_entity = NULL;
    }

    free(timerdata);
}

static
sk_timer_t* _ep_create_timer(sk_ep_t* ep,
                             sk_timer_triggered timer_cb,
                             uint32_t timeout)
{
    if (timeout == 0) return NULL;

    sk_ep_mgr_t* mgr = ep->owner;

    sk_ep_timerdata_t* timerdata = calloc(1, sizeof(*timerdata));
    timerdata->timer_entity = sk_entity_create(NULL, SK_ENTITY_EP_TIMER);
    timerdata->data.ep = ep;

    sk_ud_t cb_data  = {.ud = timerdata};
    sk_obj_opt_t opt = {.preset = NULL, .destroy = _timer_data_destroy};
    sk_obj_t* param_obj = sk_obj_create(opt, cb_data);

    sk_timer_t* timer =
        sk_timersvc_timer_create(mgr->owner->tmsvc, timerdata->timer_entity,
            timeout, 0, timer_cb, param_obj);
    SK_ASSERT(timer);

    return timer;
}

static
sk_timer_t* _ep_node_create_timer(fdlist_node_t* ep_node,
                                  sk_timer_triggered timer_cb,
                                  uint32_t timeout)
{
    if (timeout == 0) return NULL;

    sk_ep_data_t* ep_data = fdlist_get_nodedata(ep_node);
    SK_ASSERT(ep_data);
    SK_ASSERT(ep_data->owner);
    SK_ASSERT(ep_data->owner->owner);
    sk_ep_mgr_t* mgr = ep_data->owner->owner;

    sk_ep_timerdata_t* timerdata = calloc(1, sizeof(*timerdata));
    timerdata->timer_entity = sk_entity_create(NULL, SK_ENTITY_EP_TXN_TIMER);
    timerdata->data.ep_node = ep_node;

    sk_ud_t cb_data  = {.ud = timerdata};
    sk_obj_opt_t opt = {.preset = NULL, .destroy = _timer_data_destroy};
    sk_obj_t* param_obj = sk_obj_create(opt, cb_data);

    sk_timer_t* timer =
        sk_timersvc_timer_create(mgr->owner->tmsvc, timerdata->timer_entity,
            timeout, 0, timer_cb, param_obj);
    SK_ASSERT(timer);

    return timer;
}

static
sk_ep_status_t _ep_send(fdlist_node_t* ep_node, const void* data, size_t len)
{
    sk_ep_data_t* ep_data = fdlist_get_nodedata(ep_node);
    sk_ep_t* ep = ep_data->owner;
    int epfd = ep->fd;
    (void)epfd;

    if (ep->status != SK_EP_ST_CONNECTED) {
        SK_ASSERT(ep->status == SK_EP_ST_CONNECTING);
        SK_ASSERT(ep_data->status == SK_EP_NST_INIT);

        // ep still not ready (under connecting), just return
        sk_print("ep still not ready (under connecting), just return, fd: %d\n", epfd);
        ep_data->status = SK_EP_NST_PENDING;
        return SK_EP_OK;
    }

    sk_print("ep_send: send data to %s, len: %zu\n", ep->ipkey, len);
    SK_ASSERT_MSG(ep_data->status == SK_EP_NST_INIT || ep_data->status == SK_EP_NST_PENDING,
        "ep_data->status: %d\n", ep_data->status);

    ssize_t sent_bytes = sk_entity_write(ep->entity, data, len);
    if (sent_bytes < 0) {
        sk_print("ep_send data error occurred, fd: %d\n", epfd);
        return SK_EP_ERROR;
    }

    ep_data->status = SK_EP_NST_SENT;

    SK_METRICS_EP_SEND();
    SK_LOG_TRACE(SK_ENV_LOGGER, "ep send to %s, len: %zu\n", ep->ipkey, len);

    // Set up a timeout timer if needed
    if (ep_data->handler.timeout) {
        // Create or refresh shutdown timer
        _ep_create_shutdown_timer(ep, ep_data->handler.timeout * 2);

        slong_t time_left = sk_ep_txn_time_left(ep_data);
        if (time_left == 0) {
            sk_print("time left is 0 for this ep txn, set recv timeout to 1ms, alive time: %d\n",
                     (int)sk_ep_txn_time_consumed(ep_data));
            time_left = 1;
        }

        if (ep_data->handler.unpack) {
            ep_data->recv_timer =
                _ep_node_create_timer(ep_node, _recv_timeout, (uint32_t)time_left);
            SK_ASSERT(ep_data->recv_timer);

            sk_print("_ep_send: setup recv timer, timeout = %u ms\n",
                     (uint32_t)time_left);
        }
    } else {
        // If there is no unpack, we setup a shutdown timer, if has, we will
        //  setup it after receiving completed(set it in handle_ok)
        if (!ep_data->handler.unpack) {
            _ep_create_shutdown_timer(ep, SK_EP_DEFAULT_SHUTDOWN_MS);
        }
    }

    return SK_EP_OK;
}

static
void _handle_timeout(fdlist_node_t* ep_node)
{
    sk_print("handle timeout\n");
    sk_ep_data_t* ep_data = fdlist_get_nodedata(ep_node);
    int latency = (int)sk_ep_txn_time_consumed(ep_data);

    sk_ep_ret_t ret = {SK_EP_TIMEOUT, latency};
    if (ep_data->cb) {
        SK_LOG_SETCOOKIE("service.%s", sk_service_name(ep_data->service));
        SK_ENV_POS_SAVE(SK_ENV_POS_SERVICE, ep_data->service);

        ep_data->cb(ret, NULL, 0, ep_data->ud);

        SK_ENV_POS_RESTORE();
        SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    }

    _ep_node_destroy(ep_node, 1);
    SK_METRICS_EP_TIMEOUT();
}

static
void _handle_error(sk_ep_t* ep)
{
    sk_print("handle error: %s\n", strerror(errno));

    fdlist_node_t* ep_node = NULL;
    while ((ep_node = fdlist_pop(ep->txns))) {
        sk_ep_data_t* ep_data = fdlist_get_nodedata(ep_node);

        // If the ep txn data status is not INIT, means either it's in connecting
        //  or already sent, should call ep_cb, otherwise do not call it
        if (ep_data->status != SK_EP_NST_INIT) {
            int latency = (int)sk_ep_txn_time_consumed(ep_data);

            sk_print("handle_error, ep: %p, latency: %d\n", (void*)ep, latency);
            sk_ep_ret_t ret = {SK_EP_ERROR, latency};
            if (ep_data->cb) {
                SK_LOG_SETCOOKIE("service.%s", sk_service_name(ep_data->service));
                SK_ENV_POS_SAVE(SK_ENV_POS_SERVICE, ep_data->service);

                ep_data->cb(ret, NULL, 0, ep_data->ud);

                SK_ENV_POS_RESTORE();
                SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
            }

            _ep_node_destroy(ep_node, 1);
        } else {
            // Otherwise, only release the internal resources, the user resources will be released by user
            _ep_node_destroy(ep_node, 0);
        }
    }

    _ep_mgr_del(ep->owner, ep);
    SK_METRICS_EP_ERROR();
}

static
void _handle_ok(fdlist_node_t* ep_node, const void* data, size_t len)
{
    sk_ep_data_t* ep_data = fdlist_get_nodedata(ep_node);

    // Try run the ep transaction's callback
    if (ep_data->cb) {
        sk_ep_ret_t ret = {SK_EP_OK, (int)sk_ep_txn_time_consumed(ep_data)};

        SK_LOG_SETCOOKIE("service.%s", sk_service_name(ep_data->service));
        SK_ENV_POS_SAVE(SK_ENV_POS_SERVICE, ep_data->service);

        ep_data->cb(ret, data, len, ep_data->ud);

        SK_ENV_POS_RESTORE();
        SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    }

    // Setup shutdown timer
    sk_ep_t* ep = ep_data->owner;
    uint32_t timeout = ep_data->handler.timeout;
    uint32_t shutdown_timeout = timeout ? timeout : SK_EP_DEFAULT_SHUTDOWN_MS;

    _ep_create_shutdown_timer(ep, shutdown_timeout);

    // Destroy this ep transaction, then this ep can be used by other request
    _ep_node_destroy(ep_node, 1);
    SK_METRICS_EP_OK();
}

static
void _recv_timeout(sk_entity_t* entity, int valid, sk_obj_t* ud)
{
    sk_print("recv_timeout, valid: %d\n", valid);
    sk_ep_timerdata_t* timerdata = sk_obj_get(ud).ud;

    if (!valid) {
        sk_print("recv_timeout: timer invalid, skip it\n");
        goto entity_destroy;
    }

    fdlist_node_t* ep_node = timerdata->data.ep_node;
    sk_ep_data_t*  ep_data = fdlist_get_nodedata(ep_node);

    ep_data->recv_timer = NULL;
    _handle_timeout(ep_node);

entity_destroy:
    sk_entity_safe_destroy(timerdata->timer_entity);
    timerdata->timer_entity = NULL;
}

static
void _shutdown_ep(sk_entity_t* entity, int valid, sk_obj_t* ud)
{
    sk_print("_shutdown_ep, valid: %d\n", valid);
    sk_ep_timerdata_t* timerdata = sk_obj_get(ud).ud;

    if (!valid) {
        sk_print("_shutdown_ep: timer invalid, skip it\n");
        goto entity_destroy;
    }

    sk_ep_t* ep = timerdata->data.ep;
    ep->shutdown_timer = NULL;

    sk_print("Shutdown ep: alive time: %d ms, fd: %d, ntxns: %d\n",
        (int)sk_ep_time_consumed(ep), ep->fd, ep->ntxn);

    // Currently when the shutdown timer be triggered, we mark it as ERROR
    _handle_error(ep);

entity_destroy:
    sk_entity_safe_destroy(timerdata->timer_entity);
    timerdata->timer_entity = NULL;
}

static
int _try_unpack(fdlist_node_t* ep_node, void* ud)
{
    sk_ep_data_t* ep_data = fdlist_get_nodedata(ep_node);
    if (!ep_data->handler.unpack) {
        sk_print("no ep unpack function, skip it\n");
        return 0;
    }

    sk_ep_readarg_t* readarg = ud;
    const void*      data    = readarg->data;
    size_t           len     = readarg->len;
    ssize_t          consumed = -1;

    SK_LOG_SETCOOKIE("service.%s", sk_service_name(ep_data->service));
    SK_ENV_POS_SAVE(SK_ENV_POS_SERVICE, ep_data->service);

    consumed = ep_data->handler.unpack(ep_data->ud, data, len);

    SK_ENV_POS_RESTORE();
    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);

    if (consumed < 0) {
        // Error occurred
        readarg->consumed = consumed;
        return 1;
    } else if (consumed == 0) {
        // means user need more data, re-try in next round
        sk_print("user need more data, current data size=%zu\n", len);
        return 0;
    }

    // Set return consumed value
    readarg->consumed = consumed;

    // cancel the timer if needed
    if (ep_data->recv_timer) {
        sk_timer_cancel(ep_data->recv_timer);
        ep_data->recv_timer = NULL;
    }
    return 1;
}

static
void _read_cb(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_ep_t* ep = arg;
    SK_ASSERT(ep->status == SK_EP_ST_CONNECTED);
    int epfd = ep->fd;
    (void)epfd;
    sk_print("_read_cb: fd: %d\n", epfd);

    size_t read_len = 0;
    size_t used_len = fevbuff_get_usedlen(evbuff, FEVBUFF_TYPE_READ);
    if (used_len == 0) {
        read_len = SK_DEFAULT_READBUF_LEN;
    } else {
        read_len = used_len * SK_DEFAULT_READBUF_FACTOR;
    }

    ssize_t bytes = sk_entity_read(ep->entity, NULL, read_len);
    sk_print("ep entity read bytes: %ld, fd: %d\n", bytes, epfd);
    if (bytes <= 0) {
        sk_print("ep entity buffer cannot read, bytes: %d, fd: %d\n", (int)bytes, epfd);
        return;
    }

    const void* data = fevbuff_rawget(evbuff);

    sk_ep_readarg_t readarg = {
        .data     = data,
        .len      = (size_t)bytes,
        .consumed = 0
    };

    fdlist_node_t* succeed_node =
        fdlist_foreach(ep->txns, _try_unpack, &readarg);

    if (!succeed_node) {
        sk_print("There is no ep txn unpack succeed, waiting for next time\n");
        return;
    }

    ssize_t consumed = readarg.consumed;
    if (consumed < 0) {
        _handle_error(ep);
        return;
    }

    sk_print("ep txn unpack succeed, consumed: %zu\n", consumed);
    fevbuff_pop(evbuff, (size_t)consumed);

    _handle_ok(succeed_node, data, (size_t)consumed);
    SK_METRICS_EP_RECV();
    SK_LOG_TRACE(SK_ENV_LOGGER, "ep recv");
}

static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_ep_t* ep = arg;
    sk_print("_error occurred, fd: %d\n", ep->fd);
    _handle_error(ep);
}

static
int _send_each(fdlist_node_t* ep_node, void* ud)
{
    sk_print("send_each\n");
    flist* timeout_nodes = ud;
    sk_ep_data_t* ep_data = fdlist_get_nodedata(ep_node);
    SK_ASSERT_MSG(ep_data->status == SK_EP_NST_PENDING,
        "ep txn node status is not pending, somewhere wrong!!! status: %d\n", ep_data->status);

    sk_ep_status_t ret = _ep_send(ep_node, ep_data->data, ep_data->count);
    if (ret == SK_EP_TIMEOUT) {
        flist_push(timeout_nodes, ep_node);
    } else if (ret == SK_EP_ERROR) {
        return 1;
    }

    return 0;
}

static
void _on_connect(fev_state* fev, int fd, int mask, void* arg)
{
    sk_print("_on_connect\n");
    SK_LOG_TRACE(SK_ENV_LOGGER, "_on_connect");

    sk_ep_t* ep = arg;
    int sockfd = fd;

    if (mask & FEV_ERROR) {
        sk_print("fd: %d, mask: %d, error ocurred: %s\n", fd, mask, strerror(errno));
        goto CONN_ERROR;
    }

    SK_ASSERT_MSG(ep->status == SK_EP_ST_CONNECTING,
        "_on_connect status checking failed, ep->status: %d, fd: %d\n", ep->status, fd);

    int err = 0;
    socklen_t len = sizeof(int);
    if ((0 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len))) {
        if (0 == err) {
            fev_del_event(fev, sockfd, FEV_READ | FEV_WRITE);

            sk_print("target %s connected(async), ep alive time: %d, fd: %d\n",
                     ep->ipkey, (int)sk_ep_time_consumed(ep), sockfd);
            sk_ep_mgr_t* mgr = ep->owner;

            fev_buff* evbuff =
                fevbuff_new(mgr->owner->evlp, sockfd, _read_cb, _error, ep);
            SK_ASSERT(evbuff);

            sk_entity_tcp_create(ep->entity, evbuff);
            ep->status = SK_EP_ST_CONNECTED;
            SK_LOG_TRACE(SK_ENV_LOGGER, "ep connected (async)");

            if (ep->conn_timer) {
                sk_print("cancel connection timeout timer\n");
                sk_timer_cancel(ep->conn_timer);
                ep->conn_timer = NULL;
            }

            sk_print("prepare send data to target, txns size: %zu\n", fdlist_size(ep->txns));
            flist* timeout_nodes = ep->owner->tmp;
            fdlist_foreach(ep->txns, _send_each, timeout_nodes);

            fdlist_node_t* timeout_node = NULL;
            while ((timeout_node = flist_pop(timeout_nodes))) {
                _handle_timeout(timeout_node);
            }
            return;
        } else {
            sk_print("error status in connection fd: %d\n", ep->fd);
        }
    } else {
        sk_print("error in getsockopt: fd: %d, %s\n", ep->fd, strerror(errno));
    }

CONN_ERROR:
    _handle_error(ep);
}

static
void _conn_timeout(sk_entity_t* entity, int valid, sk_obj_t* ud)
{
    sk_print("conn_timeout, valid: %d\n", valid);
    sk_ep_timerdata_t* timerdata = sk_obj_get(ud).ud;

    if (!valid) {
        sk_print("connection timer not valid, skip it\n");
        goto entity_destroy;
    }

    sk_ep_t* ep = timerdata->data.ep;
    sk_print("_conn_timeout: fd: %d, status: %d\n", ep->fd, ep->status);

    if (ep->status != SK_EP_ST_CONNECTING) {
        sk_print("ep is not in connecting, skip it, status=%d\n", ep->status);
        SK_ASSERT(0);
        return;
    }

    ep->conn_timer = NULL;

    // Here mark the connection as error, since no need to wait it
    _handle_error(ep);

entity_destroy:
    sk_entity_safe_destroy(timerdata->timer_entity);
    timerdata->timer_entity = NULL;
}

// Currently only support ipv4
static
int _create_entity_tcp(sk_ep_mgr_t* mgr, const sk_ep_handler_t* handler,
                       sk_ep_t* ep, slong_t start, fdlist_node_t* ep_node)
{
    sk_ep_data_t* ep_data = fdlist_get_nodedata(ep_node);
    if (sk_ep_timeout(ep_data)) {
        sk_print("create_entity_tcp: timed out, won't connect to target\n");
        return 1;
    }

    ep->status = SK_EP_ST_CONNECTING;

    sk_print("create connection to %s, ep status: %d\n", ep->ipkey, ep->status);
    int sockfd = -1;
    int s = fnet_conn_async(handler->ip, handler->port, &sockfd);
    ep->fd = sockfd;

    sk_print("create connection to %s, ep status: %d, fd: %d, async connection status: %d\n",
        ep->ipkey, ep->status, ep->fd, s);

    if (s == 0) {
        SK_ASSERT(sockfd >= 0);
        sk_entity_t* net_entity = ep->entity;

        sk_print("ep connected to target, fd: %d\n", sockfd);
        fev_buff* evbuff =
            fevbuff_new(mgr->owner->evlp, sockfd, _read_cb, _error, ep);
        SK_ASSERT(evbuff);
        sk_entity_tcp_create(net_entity, evbuff);

        if (ep->conn_timer) {
            sk_timer_cancel(ep->conn_timer);
            ep->conn_timer = NULL;
        }

        ep->status = SK_EP_ST_CONNECTED;
        SK_LOG_TRACE(SK_ENV_LOGGER, "ep connected (sync)");
        return 0;
    } else if (s > 0) {
        SK_ASSERT(sockfd >= 0);

        if (ep_data->handler.timeout > 0) {
            slong_t time_left = sk_ep_txn_time_left(ep_data);
            if (time_left == 0) {
                sk_print("create_entity_tcp: timed out1, set connection timeout to 1ms, fd: %d\n", ep->fd);
                time_left = 1;
            }

            // Setup a connection timeout timer
            ep->conn_timer =
                _ep_create_timer(ep, _conn_timeout, (uint32_t)time_left);
            SK_ASSERT(ep->conn_timer);

            sk_print("connection setup timer: timeout: %u\n",
                     (uint32_t)time_left);
        }

        // Setup connection socket to evlp
        int ret = fev_reg_event(mgr->owner->evlp, sockfd, FEV_WRITE, NULL,
                                _on_connect, ep);
        SK_ASSERT_MSG(!ret, "Register socket into ep pool failed, ret: %d, fd: %d, max: %d\n", ret, sockfd, mgr->max);

        return 0;
    } else {
        sk_print("connect to target %s:%d failed: %s\n", handler->ip, handler->port, strerror(errno));
        return -1;
    }
}

static
int _create_entity_udp(sk_ep_mgr_t* mgr, const sk_ep_handler_t* handler,
                       sk_ep_t* ep, slong_t start, fdlist_node_t* ep_node)
{
    fnet_sockaddr_t addr;
    socklen_t addrlen = 0;
    int fd = fnet_socket(handler->ip, handler->port, SOCK_DGRAM | SOCK_NONBLOCK, &addr, &addrlen);
    if (fd < 0) {
        return -1;
    }

    ep->fd = fd;
    ep->status = SK_EP_ST_CONNECTING;
    int ret = connect(fd, &addr.sa, addrlen);
    if (ret != 0) {
        return -1;
    }

    sk_entity_t* net_entity = ep->entity;

    sk_print("ep connected to target(UDP), fd: %d\n", fd);
    fev_buff* evbuff =
        fevbuff_new(mgr->owner->evlp, fd, _read_cb, _error, ep);
    SK_ASSERT(evbuff);
    sk_entity_tcp_create(net_entity, evbuff);

    ep->status = SK_EP_ST_CONNECTED;
    return 0;
}

static
sk_ep_t* _ep_create(sk_ep_mgr_t* mgr, const sk_ep_handler_t* handler,
                    uint64_t ekey, const char* ipkey,
                    slong_t start)
{
    // Decide entity sock type
    sk_entity_type_t etype = SK_ENTITY_NONE;
    char* quote = strchr(handler->ip, ':');
    if (handler->type == SK_EP_TCP) {
        etype = !quote ? SK_ENTITY_EP_V4TCP : SK_ENTITY_EP_V6TCP;
    } else {
        etype = !quote ? SK_ENTITY_EP_V4UDP : SK_ENTITY_EP_V6UDP;
    }

    // Create sk_ep
    sk_ep_t* ep = calloc(1, sizeof(*ep));
    ep->ekey    = ekey;
    strncpy(ep->ipkey, ipkey, SK_EP_KEY_MAX);
    ep->type    = handler->type;
    ep->status  = SK_EP_ST_INIT;
    ep->owner   = mgr;
    ep->entity  = sk_entity_create(NULL, etype);
    ep->txns    = fdlist_create();
    ep->ntxn    = 0;
    ep->flags   = 0;
    ep->start   = sk_time_ms();
    ep->fd      = -1;
    SK_ASSERT(ep->entity);

    return ep;
}

static
fdlist_node_t* _ep_mgr_get_or_create(sk_ep_mgr_t*           mgr,
                                     const sk_service_t*    service,
                                     const sk_entity_t*     entity,
                                     const sk_ep_handler_t* handler,
                                     slong_t                start,
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
    if (flags & SK_EP_F_PRIVATE) {
        ekey = (uint64_t) (uintptr_t) entity;
    } else {
        ekey = 0;
    }

    ipm = fhash_u64_get(mgr->ee, ekey);
    if (!ipm) {
        ipm = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
        fhash_u64_set(mgr->ee, ekey, ipm);
    }

    char ipkey[SK_EP_KEY_MAX];
    snprintf(ipkey, SK_EP_KEY_MAX, "%s:%d", handler->ip, handler->port);
    ep_list = fhash_str_get(ipm, ipkey);
    if (!ep_list) {
        ep_list = flist_create();
        fhash_str_set(ipm, ipkey, ep_list);
    }

    ep = _find_ep(mgr, ep_list, handler);
    if (!ep) {
        ep = _ep_create(mgr, handler, ekey, ipkey, start);
        if (!ep) return NULL;

        sk_entity_mgr_add(mgr->eps, ep->entity);
        flist_push(ep_list, ep);
        mgr->nep++;

        SK_METRICS_EP_CREATE();
        SK_LOG_TRACE(SK_ENV_LOGGER, "ep create");
    }

    size_t extra_data_sz = count > EPDATA_INIT_SZ ? count - EPDATA_INIT_SZ : 0;
    // Create new ep data
    sk_ep_data_t* ep_data = calloc(1, sizeof(*ep_data) + extra_data_sz);
    ep_data->owner   = ep;
    ep_data->service = service;
    ep_data->handler = *handler;
    ep_data->cb      = cb;
    ep_data->ud      = ud;
    ep_data->start   = start;
    ep_data->e       = entity;
    ep_data->count   = count;
    ep_data->status  = SK_EP_NST_INIT;
    // Fill the data for async sending
    memcpy(ep_data->data, data, count);

    fdlist_node_t* ep_node = fdlist_make_node(ep_data, sizeof(*ep_data));
    fdlist_push(ep->txns, ep_node);
    ep->ntxn++;

    // Connect to target if needed
    if (ep->status == SK_EP_ST_INIT) {
        sk_print("ep handler type: %d\n", handler->type);
        int r = 0;
        if (handler->type == SK_EP_TCP) {
            r = _create_entity_tcp(mgr, handler, ep, start, ep_node);
        } else {
            r = _create_entity_udp(mgr, handler, ep, start, ep_node);
        }

        if (r) {
            sk_print("create entity tcp failed, fd: %d\n", ep->fd);
            _ep_node_destroy(ep_node, 0);
            _ep_mgr_del(mgr, ep);
            return NULL;
        }
    }

    return ep_node;
}

// Public APIs
sk_ep_pool_t* sk_ep_pool_create(void* evlp, sk_timersvc_t* tmsvc, int max_fds)
{
    int max = max_fds > 0 ? max_fds : SK_EP_POOL_MAX;

    sk_ep_pool_t* pool = calloc(1, sizeof(*pool));
    pool->tcp   = sk_ep_mgr_create(pool, max);
    pool->udp   = sk_ep_mgr_create(pool, max);
    pool->evlp  = evlp;
    pool->tmsvc = tmsvc;

    return pool;
}

void sk_ep_pool_destroy(sk_ep_pool_t* pool)
{
    if (!pool) return;

    sk_ep_mgr_destroy(pool->tcp);
    sk_ep_mgr_destroy(pool->udp);
    free(pool);
}

const sk_entity_mgr_t*
sk_ep_pool_emgr(const sk_ep_pool_t* pool, sk_ep_type_t type) {
    if (type == SK_EP_TCP) {
        return pool->tcp->eps;
    } else {
        return pool->udp->eps;
    }
}

sk_ep_status_t _sk_ep_send(sk_ep_pool_t*         pool,
                           const sk_service_t*   service,
                           const sk_entity_t*    entity,
                           const sk_ep_handler_t handler,
                           const void*           data,
                           size_t                count,
                           const sk_ep_cb_t      cb,
                           void*                 ud) {
    SK_ASSERT(pool);
    slong_t start = sk_time_ms();

    // 1. check
    if (!data || !count) {
        sk_print("sk_ep_send: error -> no data or data_len\n");
        SK_METRICS_EP_ERROR();
        return SK_EP_ERROR;
    }

    if (!handler.ip || handler.port == 0) {
        sk_print("sk_ep_send: error -> invalid ip or port\n");
        SK_METRICS_EP_ERROR();
        return SK_EP_ERROR;
    }

    if (handler.timeout && !handler.unpack) {
        sk_print("sk_ep_send: error-> must setup a unpack function if timeout > 0\n");
        SK_METRICS_EP_ERROR();
        return SK_EP_ERROR;
    }

    // 2. get ep mgr
    sk_ep_mgr_t* mgr = NULL;
    sk_ep_type_t et = handler.type;
    if (et == SK_EP_TCP) {
        mgr = pool->tcp;
    } else if (et == SK_EP_UDP) {
        mgr = pool->udp;
    } else {
        sk_print("sk_ep_send: error -> invalid protocol\n");
        SK_METRICS_EP_ERROR();
        return SK_EP_ERROR;
    }

    // 3. pick up one ep from ep mgr
    if (mgr->nep == mgr->max) {
        SK_METRICS_EP_ERROR();
        return SK_EP_ERROR;
    }

    fdlist_node_t* ep_node =
        _ep_mgr_get_or_create(mgr, service, entity, &handler,
                              start, cb, ud, data, count);
    if (!ep_node) {
        sk_print("sk_ep_send: error -> create ep failed\n");
        SK_METRICS_EP_ERROR();
        return SK_EP_ERROR;
    }

    // 4. send via ep
    return _ep_send(ep_node, data, count);
}

sk_ep_status_t sk_ep_send(sk_ep_pool_t*         pool,
                          const sk_service_t*   service,
                          const sk_entity_t*    entity,
                          const sk_ep_handler_t handler,
                          const void*           data,
                          size_t                count,
                          const sk_ep_cb_t      cb,
                          void*                 ud) {
    SK_ASSERT_MSG(service, "calling without service\n");
    sk_ep_status_t st = SK_EP_OK;

    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
    SK_ENV_POS_SAVE(SK_ENV_POS_CORE, NULL);

    st = _sk_ep_send(pool, service, entity, handler, data, count, cb, ud);

    SK_ENV_POS_RESTORE();
    SK_LOG_SETCOOKIE("service.%s", sk_service_name(service));
    return st;
}
