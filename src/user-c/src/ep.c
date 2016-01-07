#include <stdlib.h>

#include "api/sk_env.h"
#include "api/sk_ep_pool.h"
#include "skull/ep.h"

int skull_ep_send(skull_service_t* service, const skull_ep_handler_t handler,
                  const void* data, size_t count,
                  const skull_ep_cb_t cb, void* ud)
{
    sk_ep_handler_t sk_handler = *(sk_ep_handler_t*) &handler;

    return sk_ep_send(SK_ENV_EP, sk_handler,
                      data, count, (sk_ep_cb_t)cb, ud);
}
