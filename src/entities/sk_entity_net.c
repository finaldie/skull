#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "flibs/fev_buff.h"
#include "api/sk_utils.h"
#include "api/sk_metrics.h"
#include "api/sk_entity.h"

sk_entity_opt_t sk_entity_net_opt;

typedef struct sk_net_data_t {
    fev_buff* evbuff;
} sk_net_data_t;

void sk_entity_net_create(sk_entity_t* entity, void* evbuff)
{
    sk_net_data_t* net_data = calloc(1, sizeof(*net_data));
    net_data->evbuff = evbuff;
    sk_entity_setopt(entity, sk_entity_net_opt, net_data);

    sk_metrics_global.connection_create.inc(1);
}

static
ssize_t _net_read(sk_entity_t* entity, void* buf, size_t len, void* ud)
{
    if (!ud) return -1;

    sk_net_data_t* net_data = ud;
    fev_buff* evbuff = net_data->evbuff;

    return fevbuff_read(evbuff, buf, len);
}

static
ssize_t _net_write(sk_entity_t* entity, const void* buf, size_t len, void* ud)
{
    if (!ud) return -1;

    sk_net_data_t* net_data = ud;
    fev_buff* evbuff = net_data->evbuff;

    return fevbuff_write(evbuff, buf, len);
}

static
void _net_destroy(sk_entity_t* entity, void* ud)
{
    if (!ud) return;

    sk_print("net entity destroy\n");
    sk_metrics_global.connection_destroy.inc(1);

    sk_net_data_t* net_data = ud;
    int fd = fevbuff_destroy(net_data->evbuff);
    close(fd);
    free(net_data);
}

static
void* _net_rbufget(sk_entity_t* entity, void* ud)
{
    sk_net_data_t* net_data = ud;
    fev_buff* evbuff = net_data->evbuff;

    return fevbuff_rawget(evbuff);
}

static
size_t _net_rbufsz(const sk_entity_t* entity, void* ud)
{
    sk_net_data_t* net_data = ud;
    fev_buff* evbuff = net_data->evbuff;

    return fevbuff_get_usedlen(evbuff, FEVBUFF_TYPE_READ);
}

static
size_t _net_rbufpop(sk_entity_t* entity, size_t popsz, void* ud)
{
    sk_net_data_t* net_data = ud;
    fev_buff* evbuff = net_data->evbuff;

    return fevbuff_pop(evbuff, popsz);
}

sk_entity_opt_t sk_entity_net_opt = {
    .read    = _net_read,
    .write   = _net_write,
    .destroy = _net_destroy,

    .rbufget = _net_rbufget,
    .rbufsz  = _net_rbufsz,
    .rbufpop = _net_rbufpop
};
