#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "flibs/fnet.h"
#include "flibs/fev.h"
#include "flibs/fev_listener.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_driver.h"

typedef struct sk_driver_tcp_data_t {
    in_port_t port;
    uint32_t  is_v4     :1;
    uint32_t  _reserved :15;

    int listen_fd;
    fev_listen_info* listener;
} sk_driver_tcp_data_t;

// this is running on the main scheduler io thread
// NOTES: The driver is responsible for creating a base entity object
static
void _sk_accept(fev_state* fev, int fd, void* ud)
{
    if (fd < 0) {
        sk_print("new accepted fd(%d) < 0, won't create an entity\n", fd);
        return;
    }

    if (-1 == fnet_set_nonblocking(fd)) {
        fprintf(stderr, "set nonblocking to socket fd %d failed, won't create an entity\n", fd);
        close(fd);
        return;
    }

    sk_driver_t*  driver  = ud;
    sk_engine_t*   engine   = driver->engine;
    sk_workflow_t* workflow = driver->workflow;
    sk_sched_t*    sched    = engine->sched;
    bool           is_v4    = ((sk_driver_tcp_data_t*)driver->data)->is_v4;

    sk_entity_t* entity = sk_entity_create(workflow,
                           is_v4 ? SK_ENTITY_SOCK_V4TCP : SK_ENTITY_SOCK_V6TCP);
    sk_print("create a new entity(%d)\n", fd);

    NetAccept accept_msg = NET_ACCEPT__INIT;
    accept_msg.fd = fd;
    sk_sched_send(sched, NULL, entity, NULL, SK_PTO_NET_ACCEPT, &accept_msg, 0);
}

static
void _driver_tcp_create(sk_driver_t* driver)
{
    const sk_workflow_cfg_t* cfg = driver->workflow->cfg;
    SK_ASSERT_MSG(cfg->port > 0 && cfg->port <= 65535,
                  "driver port(%d) must be in (0, 65535]\n", cfg->port);

    sk_driver_tcp_data_t* data = calloc(1, sizeof(*data));
    data->port      = (in_port_t)cfg->port;
    data->listen_fd = fnet_listen(cfg->bind, data->port, SK_MAX_LISTEN_BACKLOG, 0);
    data->is_v4     = !strchr(cfg->bind, ':');

    SK_ASSERT_MSG(data->listen_fd >= 0, "Cannot listen to %s: %s\n", cfg->bind, strerror(errno));

    driver->data = data;
}

static
void _driver_tcp_run(sk_driver_t* driver)
{
    sk_driver_tcp_data_t* data = driver->data;
    sk_engine_t* engine    = driver->engine;
    fev_state*   fev       = engine->evlp;
    int          listen_fd = data->listen_fd;

    data->listener = fev_add_listener_byfd(fev, listen_fd, _sk_accept, driver);
    SK_ASSERT(data->listener);
}

static
void _driver_tcp_destroy(sk_driver_t* driver)
{
    sk_driver_tcp_data_t* data = driver->data;
    // notes: no need to close listen_fd again, since fev_del_listener has
    // already does it
    fev_del_listener(driver->engine->evlp, data->listener);
    free(driver->data);
}

sk_driver_opt_t sk_driver_tcp = {
    .create  = _driver_tcp_create,
    .run     = _driver_tcp_run,
    .destroy = _driver_tcp_destroy
};
