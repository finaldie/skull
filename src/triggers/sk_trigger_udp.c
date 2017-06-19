#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "flibs/fnet.h"
#include "flibs/fev.h"
#include "flibs/fev_buff.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_trigger.h"
#include "api/sk_entity_util.h"
#include "api/sk_trigger_utils.h"

typedef struct sk_trigger_udp_data_t {
    in_port_t port;
    short     _reserved;
    int       rootfd;

    fnet_sockaddr_t addr;
    socklen_t       addrlen;
    int             _reserved1;

    char      readbuf[UINT16_MAX + 1];
} sk_trigger_udp_data_t;

/**
 * This is running on the main scheduler io thread
 *
 * @note The trigger is responsible for creating a base entity object (UDP)
 *       The 'concurrent' flag is not applicable for UDP entity
 */
static
void _readcb(fev_state* fev, int fd, int mask, void* ud)
{
    if (mask & FEV_ERROR) {
        sk_print("UDP _readcb error\n");
        return;
    }

    fnet_sockaddr_t c_addr;
    memset(&c_addr, 0, sizeof(c_addr));
    socklen_t c_addr_len = sizeof(c_addr);

    sk_trigger_t* trigger = ud;
    sk_trigger_udp_data_t* data = trigger->data;
    char* readbuf = data->readbuf;

    ssize_t bytes = recvfrom(fd, readbuf, UINT16_MAX, 0, &c_addr.sa, &c_addr_len);

    if (bytes < 0) { // Error occurred
        sk_print("UDP _readcb: recvfrom return value < 0, skip to trigger workflow\n");
        return;
    } else if (bytes == 0) { // Shutdowned
        sk_print("UDP _readcb: recvfrom return value = 0, skip to trigger workflow\n");
        return;
    }

    // Everything is normal, create an entity
    sk_workflow_t* workflow = trigger->workflow;
    sk_entity_t*   entity   = sk_entity_create(workflow,
                                data->addr.sa.sa_family == AF_INET
                                  ? SK_ENTITY_SOCK_V4UDP
                                  : SK_ENTITY_SOCK_V6UDP);

    // Set entity opt
    sk_entity_udp_create(entity, fd, readbuf, (uint16_t)bytes, &c_addr.sa, c_addr_len);

    // Unpack data and trigger workflow if possible
    sk_trigger_util_unpack(entity, NULL);
}

static
void _trigger_udp_create(sk_trigger_t* trigger)
{
    const sk_workflow_cfg_t* cfg = trigger->workflow->cfg;
    SK_ASSERT_MSG(cfg->port > 0 && cfg->port <= 65535,
                  "trigger port(%d) must be in (0, 65535]\n", cfg->port);

    fnet_sockaddr_t addr;
    socklen_t addrlen = 0;
    int rootfd = fnet_socket(cfg->bind, (in_port_t)cfg->port,
                             SOCK_DGRAM | SOCK_NONBLOCK, &addr, &addrlen);
    SK_ASSERT_MSG(rootfd >= 0, "Create UDP rootfd failed, error: %d, %s",
                  errno, strerror(errno));

    if (fnet_set_reuse_addr(rootfd)) {
        SK_ASSERT_MSG(0, "Create set SO_REUSEADDR option");
    }

    sk_trigger_udp_data_t* data = calloc(1, sizeof(*data));
    data->port    = (in_port_t)cfg->port;
    data->rootfd  = rootfd;
    data->addr    = addr;
    data->addrlen = addrlen;

    trigger->data = data;
}

static
void _trigger_udp_run(sk_trigger_t* trigger)
{
    const sk_workflow_cfg_t* cfg = trigger->workflow->cfg; (void)cfg;
    sk_trigger_udp_data_t* data  = trigger->data;

    sk_engine_t* engine = trigger->engine;
    fev_state*   fev    = engine->evlp;
    int          rootfd = data->rootfd;

    if (bind(rootfd, &data->addr.sa, data->addrlen)) {
        SK_ASSERT_MSG(0, "Bind UDP rootfd failed, error: %d, %s",
                  errno, strerror(errno));
    }

    int ret = fev_reg_event(fev, rootfd, FEV_READ, _readcb, NULL, trigger);
    if (ret) {
        SK_ASSERT_MSG(ret == 0, "Register UDP rootfd failed, ret: %d\n", ret);
    }
}

static
void _trigger_udp_destroy(sk_trigger_t* trigger)
{
    sk_trigger_udp_data_t* data = trigger->data;
    fev_state* fev = trigger->engine->evlp;

    fev_del_event(fev, data->rootfd, FEV_READ | FEV_WRITE);
    close(data->rootfd);
    free(trigger->data);
}

sk_trigger_opt_t sk_trigger_udp = {
    .create  = _trigger_udp_create,
    .run     = _trigger_udp_run,
    .destroy = _trigger_udp_destroy
};
