#ifndef SK_EP_POOL_H
#define SK_EP_POOL_H

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <netinet/in.h>

#include "api/sk_service.h"
#include "api/sk_timer_service.h"

typedef struct sk_ep_pool_t sk_ep_pool_t;

sk_ep_pool_t* sk_ep_pool_create(void* evlp, sk_timersvc_t*, int max);
void sk_ep_pool_destroy(sk_ep_pool_t*);

typedef enum sk_ep_type_t {
    SK_EP_TCP = 0,
    SK_EP_UDP = 1
} sk_ep_type_t;

typedef enum sk_ep_status_t {
    SK_EP_OK          = 0,
    SK_EP_ERROR       = 1,
    SK_EP_TIMEOUT     = 2,
} sk_ep_status_t;

typedef struct sk_ep_ret_t {
    sk_ep_status_t status;
    int            latency;
} sk_ep_ret_t;

typedef void (*sk_ep_cb_t) (sk_ep_ret_t, const void* response,
                            size_t len, void* ud);

#define SK_EP_F_CONCURRENT 0x1
#define SK_EP_F_PRIVATE    0x2

typedef struct sk_ep_handler_t {
    sk_ep_type_t type;
    in_port_t    port;
    uint16_t     _reserved;

    const char*  ip;

    // unit: millisecond
    //   0: means no timeout
    // > 0: after x milliseconds, the ep call would time out
    uint32_t     timeout;

    // Set value of 'SK_EP_F_CONCURRENT' and 'SK_EP_F_PRIVATE'
    // Notes: currently these flags are no effect
    int          flags;

    /**
     * Unpack endpoint response
     *
     * @return - (< 0): Error occurred
     *         - (= 0): The response data has not finished yet
     *         - (> 0): The response data has finished
     */
    ssize_t      (*unpack)  (void* ud, const void* data, size_t len);

    /**
     * Release user resource
     */
    void         (*release) (void* ud);
} sk_ep_handler_t;

/**
 * Pick up a end point and send the data out
 */
sk_ep_status_t sk_ep_send(sk_ep_pool_t*         pool,
                          const sk_service_t*   service,
                          const sk_entity_t*    entity,
                          const sk_ep_handler_t handler,
                          const void*           data,
                          size_t                count,
                          const sk_ep_cb_t      cb,
                          void*                 ud);

struct sk_entity_mgr_t;

const struct sk_entity_mgr_t*
sk_ep_pool_emgr(const sk_ep_pool_t*, sk_ep_type_t);

#endif

