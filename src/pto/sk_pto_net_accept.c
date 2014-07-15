#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "fev/fev.h"
#include "fev/fev_buff.h"
#include "api/sk_assert.h"
#include "api/sk_event.h"
#include "api/sk_sched.h"

// read buffer and create a sk_event
// TODO: need to create a client mgr
// TODO: need to create a context {sched, evbuff, ...}
static
void _read(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_sched_t* sched = arg;
    void* data = malloc(1024);
    int bytes = fevbuff_read(evbuff, data, 1024);
    if (bytes <= 0) {
        printf("buffer cannot read\n");
        return;
    }

    sk_event_t event;
    event.ev_type = SK_EV_INCOMING;
    event.type    = SK_EV_REQ;
    event.deliver = SK_IO_NET_SOCK;
    event.pto_id  = SK_PTO_NET_PROC;
    event.data.d.ud = data;
    event.data.sz = bytes;
    sk_sched_push(sched, &event);
}

static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    int fd = fevbuff_destroy(evbuff);
    close(fd);
}

// register the new sock fd into eventloop
static
int _req(sk_sched_t* sched, sk_event_t* event)
{
    printf("event req\n");
    fev_state* fev = sk_sched_get_eventloop(sched);
    int client_fd = event->data.d.u32;
    fev_buff* evbuff = fevbuff_new(fev, client_fd, _read, _error, sched);
    SK_ASSERT(evbuff);
    return 0;
}

static
int _end(sk_sched_t* sched, sk_event_t* event)
{
    printf("event end\n");
    return 1;
}

sk_event_opt_t sk_pto_net_accept = {
    .req = _req,
    .resp = NULL,
    .end = _end,
    .destroy = NULL
};
