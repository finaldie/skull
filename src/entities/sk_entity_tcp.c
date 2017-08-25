#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "flibs/fev_buff.h"
#include "flibs/fnet.h"
#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_entity.h"

sk_entity_opt_t sk_entity_tcp_opt;

typedef struct sk_tcp_data_t {
    fev_buff* evbuff;
} sk_tcp_data_t;

void sk_entity_tcp_create(sk_entity_t* entity, void* evbuff)
{
    sk_tcp_data_t* tdata = calloc(1, sizeof(*tdata));
    tdata->evbuff = evbuff;
    sk_entity_setopt(entity, sk_entity_tcp_opt, tdata);

    sk_metrics_global.connection_create.inc(1);
}

static
ssize_t _tcp_read(sk_entity_t* entity, void* buf, size_t len, void* ud)
{
    if (!ud) return -1;

    sk_tcp_data_t* tdata = ud;
    fev_buff* evbuff = tdata->evbuff;

    return fevbuff_read(evbuff, buf, len);
}

static
ssize_t _tcp_write(sk_entity_t* entity, const void* buf, size_t len, void* ud)
{
    if (!ud) return -1;

    sk_tcp_data_t* tdata = ud;
    fev_buff* evbuff = tdata->evbuff;

    return fevbuff_write(evbuff, buf, len);
}

static
void _tcp_destroy(sk_entity_t* entity, void* ud)
{
    if (!ud) return;

    sk_print("tcp entity destroy\n");
    sk_metrics_global.connection_destroy.inc(1);

    sk_tcp_data_t* tdata = ud;
    int fd = fevbuff_destroy(tdata->evbuff);
    close(fd);
    free(tdata);
}

static
void* _tcp_rbufget(const sk_entity_t* entity, void* ud)
{
    sk_tcp_data_t* tdata = ud;
    fev_buff* evbuff = tdata->evbuff;

    return fevbuff_rawget(evbuff);
}

static
size_t _tcp_rbufsz(const sk_entity_t* entity, void* ud)
{
    sk_tcp_data_t* tdata = ud;
    fev_buff* evbuff = tdata->evbuff;

    return fevbuff_get_usedlen(evbuff, FEVBUFF_TYPE_READ);
}

static
size_t _tcp_rbufpop(sk_entity_t* entity, size_t popsz, void* ud)
{
    sk_tcp_data_t* tdata = ud;
    fev_buff* evbuff = tdata->evbuff;

    return fevbuff_pop(evbuff, popsz);
}

static
int _tcp_peer(const sk_entity_t* entity, sk_entity_peer_t* peer, void* ud)
{
    sk_tcp_data_t* tdata = ud;
    int fd = fevbuff_get_fd(tdata->evbuff);

    fnet_peername(fd, peer->name, INET6_ADDRSTRLEN, &peer->port, &peer->family);
    return 0;
}

sk_entity_opt_t sk_entity_tcp_opt = {
    .read    = _tcp_read,
    .write   = _tcp_write,
    .destroy = _tcp_destroy,

    .rbufget = _tcp_rbufget,
    .rbufsz  = _tcp_rbufsz,
    .rbufpop = _tcp_rbufpop,
    .peer    = _tcp_peer
};
