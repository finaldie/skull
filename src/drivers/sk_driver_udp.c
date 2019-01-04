#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "flibs/fnet.h"
#include "flibs/fev.h"
#include "flibs/fev_buff.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_driver.h"
#include "api/sk_entity_util.h"
#include "api/sk_driver_utils.h"

typedef struct sk_driver_udp_data_t {
    in_port_t port;
    short     _reserved;
    int       rootfd;

    fnet_sockaddr_t addr;
    socklen_t       addrlen;
    int             _reserved1;

    char      readbuf[UINT16_MAX + 1];
} sk_driver_udp_data_t;

/**
 * This is running on the main scheduler io thread
 *
 * @note The driver is responsible for creating a base entity object (UDP)
 *       The 'concurrency' flag is not applicable for UDP entity, which means
 *        it's always driver a concurrency workflow
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

    sk_driver_t* driver = ud;
    sk_driver_udp_data_t* data = driver->data;
    char* readbuf = data->readbuf;

    ssize_t bytes = recvfrom(fd, readbuf, UINT16_MAX, 0, &c_addr.sa, &c_addr_len);

    if (bytes < 0) { // Error occurred
        sk_print("UDP _readcb: recvfrom return value < 0, skip to driver workflow\n");
        return;
    }

    if (bytes == 0) { // Shutdowned
        sk_print("UDP _readcb: recvfrom return value = 0, skip to driver workflow\n");
        return;
    }

    // Everything is normal, create an entity
    sk_workflow_t* workflow = driver->workflow;
    sk_entity_t*   entity   = sk_entity_create(workflow,
                                data->addr.sa.sa_family == AF_INET
                                  ? SK_ENTITY_SOCK_V4UDP
                                  : SK_ENTITY_SOCK_V6UDP);

    // Set entity opt
    sk_entity_udp_create(entity, fd, readbuf, (uint16_t)bytes, &c_addr.sa, c_addr_len);

    // Unpack data and driver workflow if possible
    ssize_t consumed = sk_driver_util_unpack(entity);
    if (consumed > 0) {
        // Parsed a valid UDP packet, deliver it to workflow
        int cnt = sk_driver_util_deliver(entity, NULL, 1);
        SK_ASSERT(cnt == 1);
    }
}

static
void _driver_udp_create(sk_driver_t* driver)
{
    const sk_workflow_cfg_t* cfg = driver->workflow->cfg;
    SK_ASSERT_MSG(cfg->port > 0 && cfg->port <= 65535,
                  "driver port(%d) must be in (0, 65535]\n", cfg->port);

    fnet_sockaddr_t addr;
    socklen_t addrlen = 0;
    int rootfd = fnet_socket(cfg->bind, (in_port_t)cfg->port,
                             SOCK_DGRAM | SOCK_NONBLOCK, &addr, &addrlen);
    SK_ASSERT_MSG(rootfd >= 0, "Create UDP rootfd failed, error: %d, %s",
                  errno, strerror(errno));

    if (fnet_set_reuse_addr(rootfd)) {
        SK_ASSERT_MSG(0, "Create set SO_REUSEADDR option");
    }

    sk_driver_udp_data_t* data = calloc(1, sizeof(*data));
    data->port    = (in_port_t)cfg->port;
    data->rootfd  = rootfd;
    data->addr    = addr;
    data->addrlen = addrlen;

    driver->data  = data;
}

static
void _driver_udp_run(sk_driver_t* driver)
{
    const sk_workflow_cfg_t* cfg = driver->workflow->cfg; (void)cfg;
    sk_driver_udp_data_t* data  = driver->data;

    sk_engine_t* engine = driver->engine;
    fev_state*   fev    = engine->evlp;
    int          rootfd = data->rootfd;

    if (bind(rootfd, &data->addr.sa, data->addrlen)) {
        SK_ASSERT_MSG(0, "Bind UDP rootfd failed, error: %d, %s",
                  errno, strerror(errno));
    }

    int ret = fev_reg_event(fev, rootfd, FEV_READ, _readcb, NULL, driver);
    SK_ASSERT_MSG(ret == 0, "Register UDP rootfd failed, ret: %d\n", ret);
}

static
void _driver_udp_destroy(sk_driver_t* driver)
{
    sk_driver_udp_data_t* data = driver->data;
    fev_state* fev = driver->engine->evlp;

    fev_del_event(fev, data->rootfd, FEV_READ | FEV_WRITE);
    close(data->rootfd);
    free(driver->data);
}

sk_driver_opt_t sk_driver_udp = {
    .create  = _driver_udp_create,
    .run     = _driver_udp_run,
    .destroy = _driver_udp_destroy
};
