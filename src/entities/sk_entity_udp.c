#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "flibs/fnet.h"
#include "flibs/fev_buff.h"
#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_entity.h"

#define SK_PADDING_SZ (2) // 2 = short

sk_entity_opt_t sk_entity_udp_opt;

static size_t _udp_rbufsz(const sk_entity_t* entity, void* ud);

typedef struct sk_udp_data_t {
    int       rootfd;
    uint16_t  buf_sz;
    uint16_t  _padding;

    fnet_sockaddr_t src_addr;
    socklen_t src_addr_len;

    uint16_t  sidx;
    char      readbuf[SK_PADDING_SZ];
} sk_udp_data_t;

void sk_entity_udp_create(sk_entity_t* entity, int rootfd,
                          const void* buf, uint16_t buf_sz,
                          struct sockaddr* src_addr, socklen_t src_addr_len)
{
    size_t extra_sz = buf_sz > SK_PADDING_SZ ? buf_sz - (size_t)SK_PADDING_SZ : 0;

    sk_udp_data_t* udp_data = calloc(1, sizeof(*udp_data) + extra_sz);
    udp_data->rootfd        = rootfd;
    udp_data->buf_sz        = buf_sz;
    udp_data->src_addr_len  = src_addr_len;
    memcpy(&udp_data->src_addr, src_addr, src_addr_len);
    memcpy(udp_data->readbuf, buf, buf_sz);

    sk_entity_setopt(entity, sk_entity_udp_opt, udp_data);
    sk_entity_setflags(entity, sk_entity_flags(entity) | SK_ENTITY_F_DESTROY_NOTXN);
}

static
ssize_t _udp_read(sk_entity_t* entity, void* buf, size_t len, void* ud)
{
    if (!ud) return -1;

    sk_udp_data_t* udp_data = ud;
    size_t rbufsz  = _udp_rbufsz(entity, ud);
    size_t readlen = len < rbufsz ? len : rbufsz;
    if (readlen == 0) return 0; // Nothing can be read

    if (buf) memcpy(buf, udp_data->readbuf + udp_data->sidx, readlen);
    return (ssize_t)readlen;
}

static
ssize_t _udp_write(sk_entity_t* entity, const void* buf, size_t len, void* ud)
{
    if (!ud) return -1;

    sk_udp_data_t* udp_data = ud;
    return sendto(udp_data->rootfd, buf, len, MSG_NOSIGNAL, &udp_data->src_addr.sa,
                  udp_data->src_addr_len);
}

static
void _entity_destroy(sk_entity_t* entity, void* ud)
{
    if (!ud) return;
    sk_print("udp entity destroy\n");
    free(ud);
}

static
void* _udp_rbufget(const sk_entity_t* entity, void* ud)
{
    sk_udp_data_t* udp_data = ud;
    return udp_data->readbuf + udp_data->sidx;
}

static
size_t _udp_rbufsz(const sk_entity_t* entity, void* ud)
{
    sk_udp_data_t* udp_data = ud;
    return (size_t)(udp_data->buf_sz - udp_data->sidx);
}

static
size_t _udp_rbufpop(sk_entity_t* entity, size_t popsz, void* ud)
{
    sk_udp_data_t* udp_data = ud;
    size_t rbufsz = _udp_rbufsz(entity, ud);
    size_t final_popsz = popsz < rbufsz ? popsz : rbufsz;

    udp_data->sidx = (uint16_t)(udp_data->sidx + final_popsz);
    return final_popsz;
}

static
int _udp_peer(const sk_entity_t* entity, sk_entity_peer_t* peer, void* ud)
{
    sk_udp_data_t* udp_data = ud;

    peer->family = udp_data->src_addr.sa.sa_family;
    if (sk_entity_type(entity) == SK_ENTITY_SOCK_V4TCP) {
        inet_ntop(AF_INET, (void*)&udp_data->src_addr.in.sin_addr, peer->name, INET6_ADDRSTRLEN);
        peer->port = ntohs(udp_data->src_addr.in.sin_port);
    } else {
        inet_ntop(AF_INET6, (void*)&udp_data->src_addr.in6.sin6_addr, peer->name, INET6_ADDRSTRLEN);
        peer->port = ntohs(udp_data->src_addr.in6.sin6_port);
    }

    return 0;
}

sk_entity_opt_t sk_entity_udp_opt = {
    .read    = _udp_read,
    .write   = _udp_write,
    .destroy = _entity_destroy,

    .rbufget = _udp_rbufget,
    .rbufsz  = _udp_rbufsz,
    .rbufpop = _udp_rbufpop,
    .peer    = _udp_peer
};
