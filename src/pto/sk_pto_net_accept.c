#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "fev/fev.h"
#include "fev/fev_buff.h"
#include "api/sk_assert.h"
#include "api/sk_event.h"
#include "api/sk_sched.h"

typedef struct sk_net_data_t {
    fev_buff* evbuff;
} sk_net_data_t;

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
void net_error(sk_entity_t* entity, void* ud)
{
}

static
void net_destroy(sk_entity_t* entity, void* ud)
{
    printf("net_destroy\n");
    sk_net_data_t* net_data = ud;
    fevbuff_destroy(net_data->evbuff);
    free(net_data);
}

sk_entity_opt_t net_opt = {
    .read = net_read,
    .write = net_write,
    .error = net_error,
    .destroy = net_destroy
};

// -----------------------------------------------------------------------------

// EventLoop trigger this callback
// read buffer and create a sk_event
// TODO: sk_event should use protobuf to store the data
static
void _read_cb(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_entity_t* entity = arg;
    sk_entity_mgr_t* entity_mgr = sk_entity_owner(entity);
    sk_sched_t* sched = sk_entity_mgr_owner(entity_mgr);

    void* data = calloc(1, 1024);
    int bytes = sk_entity_read(entity, data, 1024);
    if (bytes <= 0) {
        printf("buffer cannot read\n");
        free(data);
        return;
    }

    sk_event_t event;
    event.deliver = SK_IO_NET_SOCK;
    event.ev_type = SK_EV_INCOMING;
    event.pto_id  = SK_PTO_NET_PROC;
    event.entity  = entity;
    event.sz = bytes;
    event.data.ud = data;
    sk_sched_push(sched, &event);

    // consume the evbuff
    fevbuff_pop(evbuff, bytes);
}

static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    printf("evbuff destroy...\n");
    sk_entity_t* entity = arg;
    sk_entity_mgr_t* entity_mgr = sk_entity_owner(entity);

    sk_entity_mgr_del(entity_mgr, entity);
}

// register the new sock fd into eventloop
static
int _req(sk_sched_t* sched, sk_event_t* event)
{
    printf("event req\n");
    fev_state* fev = sk_sched_get_eventloop(sched);
    int client_fd = event->data.u32;
    sk_entity_mgr_t* entity_mgr = sk_sched_entity_mgr(sched);
    sk_net_data_t* net_data = malloc(sizeof(*net_data));

    sk_entity_t* entity = sk_entity_create(client_fd, net_data, net_opt);
    fev_buff* evbuff = fevbuff_new(fev, client_fd, _read_cb, _error, entity);
    SK_ASSERT(evbuff);

    net_data->evbuff = evbuff;
    sk_entity_mgr_add(entity_mgr, entity);
    return 0;
}

static
int _req_end(sk_sched_t* sched, sk_event_t* event)
{
    printf("req event end\n");
    return 1;
}

sk_event_opt_t sk_pto_net_accept = {
    .req  = _req,
    .resp = NULL,
    .req_end  = _req_end,
    .resp_end = NULL,
    .destroy  = NULL
};
