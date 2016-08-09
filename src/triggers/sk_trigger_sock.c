#include <stdlib.h>
#include <string.h>

#include "flibs/fnet.h"
#include "flibs/fev.h"
#include "flibs/fev_listener.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_trigger.h"

typedef struct sk_trigger_sock_data_t {
    in_port_t port;
    short     _reserved;

    int listen_fd;
    fev_listen_info* listener;
} sk_trigger_sock_data_t;

// this is running on the main scheduler io thread
// NOTES: The trigger is responsible for creating a base entity object
static
void _sk_accept(fev_state* fev, int fd, void* ud)
{
    sk_trigger_t*  trigger  = ud;
    sk_engine_t*   engine   = trigger->engine;
    sk_workflow_t* workflow = trigger->workflow;
    sk_sched_t*    sched    = engine->sched;

    sk_entity_t* entity = sk_entity_create(workflow, SK_ENTITY_TAG_NET);
    sk_print("create a new entity(%d)\n", fd);

    NetAccept accept_msg = NET_ACCEPT__INIT;
    accept_msg.fd = fd;
    sk_sched_send(sched, NULL, entity, NULL, SK_PTO_NET_ACCEPT, &accept_msg, 0);
}

static
void _trigger_sock_create(sk_trigger_t* trigger, sk_workflow_cfg_t* cfg)
{
    SK_ASSERT_MSG(cfg->port > 0 && cfg->port <= 65535,
                  "trigger port(%d) must be in (0, 65535]\n", cfg->port);

    sk_trigger_sock_data_t* data = calloc(1, sizeof(*data));
    data->port = (in_port_t)cfg->port;
    data->listen_fd = !cfg->localhost
                          ? fnet_listen(NULL, data->port, 1024, 0)
                          : fnet_listen("127.0.0.1", data->port, 1024, 0);

    trigger->data = data;
}

static
void _trigger_sock_run(sk_trigger_t* trigger)
{
    sk_trigger_sock_data_t* data = trigger->data;
    sk_engine_t* engine    = trigger->engine;
    fev_state*   fev       = engine->evlp;
    int          listen_fd = data->listen_fd;

    data->listener = fev_add_listener_byfd(fev, listen_fd, _sk_accept, trigger);
    SK_ASSERT(data->listener);
}

static
void _trigger_sock_destroy(sk_trigger_t* trigger)
{
    sk_trigger_sock_data_t* data = trigger->data;
    // notes: no need to close listen_fd again, since fev_del_listener has
    // already does it
    fev_del_listener(trigger->engine->evlp, data->listener);
    free(trigger->data);
}

sk_trigger_opt_t sk_trigger_sock = {
    .create  = _trigger_sock_create,
    .run     = _trigger_sock_run,
    .destroy = _trigger_sock_destroy
};
