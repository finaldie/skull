#include <stdio.h>
#include <stdlib.h>

#include "fev/fev_buff.h"
#include "api/sk_utils.h"
#include "api/sk_entity_mgr.h"

sk_entity_opt_t sk_net_entity_opt;

typedef struct sk_net_data_t {
    fev_buff* evbuff;
} sk_net_data_t;

void sk_net_entity_create(sk_entity_t* entity, struct fev_buff* evbuff)
{
    sk_net_data_t* net_data = malloc(sizeof(*net_data));
    net_data->evbuff = evbuff;
    sk_entity_setopt(entity, sk_net_entity_opt, net_data);
}

static
int net_read(sk_entity_t* entity, void* buf, int len, void* ud)
{
    sk_net_data_t* net_data = ud;
    fev_buff* evbuff = net_data->evbuff;

    return fevbuff_read(evbuff, buf, len);
}

static
int net_write(sk_entity_t* entity, const void* buf, int len, void* ud)
{
    sk_net_data_t* net_data = ud;
    fev_buff* evbuff = net_data->evbuff;

    return fevbuff_write(evbuff, buf, len);
}

static
void net_destroy(sk_entity_t* entity, void* ud)
{
    sk_print("net_destroy\n");
    sk_net_data_t* net_data = ud;
    fevbuff_destroy(net_data->evbuff);
    free(net_data);
}

sk_entity_opt_t sk_net_entity_opt = {
    .read = net_read,
    .write = net_write,
    .destroy = net_destroy
};
