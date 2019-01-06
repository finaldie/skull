/**
 * Generic entity can operate on a fevbuff
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "flibs/compiler.h"
#include "flibs/fev_buff.h"
#include "flibs/fnet.h"
#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_entity.h"

sk_entity_opt_t sk_entity_evb_ops;

void sk_entity_evb_create(sk_entity_t* entity, void* evbuff) {
    sk_entity_setopt(entity, sk_entity_evb_ops, evbuff);
}

static
ssize_t _evb_read(sk_entity_t* entity, void* buf, size_t len, void* ud) {
    if (unlikely(!ud)) return -1;
    return fevbuff_read((fev_buff*)ud, buf, len);
}

static
ssize_t _evb_write(sk_entity_t* entity, const void* buf, size_t len, void* ud) {
    if (unlikely(!ud)) return -1;
    return fevbuff_write((fev_buff*)ud, buf, len);
}

static
void _evb_destroy(sk_entity_t* entity, void* ud) {
    if (unlikely(!ud)) return;

    int fd = fevbuff_destroy((fev_buff*)ud);
    close(fd);
}

static
void* _evb_rbufget(const sk_entity_t* entity, void* ud) {
    return fevbuff_rawget((fev_buff*)ud);
}

static
size_t _evb_rbufsz(const sk_entity_t* entity, void* ud) {
    if (unlikely(!ud)) return 0;
    return fevbuff_get_usedlen((fev_buff*)ud, FEVBUFF_TYPE_READ);
}

static
size_t _evb_rbufpop(sk_entity_t* entity, size_t popsz, void* ud) {
    if (unlikely(!ud)) return 0;
    return fevbuff_pop((fev_buff*)ud, popsz);
}

static
int _evb_peer(const sk_entity_t* entity, sk_entity_peer_t* peer, void* ud) {
    if (unlikely(!ud)) return -1;
    int fd = fevbuff_get_fd((fev_buff*)ud);

    fnet_peername(fd, peer->name, INET6_ADDRSTRLEN, &peer->port, &peer->family);
    return 0;
}

sk_entity_opt_t sk_entity_evb_ops = {
    .read    = _evb_read,
    .write   = _evb_write,
    .destroy = _evb_destroy,

    .rbufget = _evb_rbufget,
    .rbufsz  = _evb_rbufsz,
    .rbufpop = _evb_rbufpop,
    .peer    = _evb_peer
};
