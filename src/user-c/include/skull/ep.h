#ifndef SKULL_EP_H
#define SKULL_EP_H

#include <stddef.h>
#include <netinet/in.h>

#include "skull/service.h"

typedef enum skull_ep_type_t {
    SKULL_EP_TCP = 0,
    SKULL_EP_UDP = 1
} skull_ep_type_t;

typedef enum skull_ep_status_t {
    SKULL_EP_OK          = 0,
    SKULL_EP_ERROR       = 1,
    SKULL_EP_TIMEOUT     = 2,
} skull_ep_status_t;

typedef struct skull_ep_ret_t {
    skull_ep_status_t status;
    int               latency;
} skull_ep_ret_t;

typedef struct skull_ep_handler_t {
    skull_ep_type_t type;
    in_port_t    port;
    uint16_t     _reserved;

    const char*  ip;
    int          timeout; // unit: millisecond
    int          flags;

    // return 0:   The response data has not finished yet
    // return > 0: The response data has finished
    size_t       (*unpack)  (void* ud, const void* data, size_t len);
    void         (*release) (void* ud);
} skull_ep_handler_t;

typedef void (*skull_ep_cb_t) (skull_service_t*, skull_ep_ret_t,
                               const void* response, size_t len, void* ud,
                               const void* api_req, void* api_resp);

skull_ep_status_t
skull_ep_send(skull_service_t*, const skull_ep_handler_t handler,
              const void* data, size_t count,
              const skull_ep_cb_t cb, void* ud);

#endif

