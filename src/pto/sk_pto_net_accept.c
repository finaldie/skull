#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "fev/fev.h"
#include "fev/fev_buff.h"
#include "api/sk_utils.h"
#include "api/sk_event.h"
#include "api/sk_pto.h"
#include "api/sk_sched.h"

// -----------------------------------------------------------------------------

// EventLoop trigger this callback
// read buffer and create a sk_event
// TODO: sk_event should use protobuf to store the data
static
void _read_cb(fev_state* fev, fev_buff* evbuff, void* arg)
{
    sk_entity_t* entity = arg;
    sk_sched_t* sched = sk_entity_sched(entity);
    int fd = sk_entity_fd(entity);

    // calculate how many bytes we need to read
    size_t used_len = fevbuff_get_usedlen(evbuff, FEVBUFF_TYPE_READ);
    int read_len = 0;
    if (used_len == 0) {
        read_len = 1024;
    } else {
        read_len *= used_len * 2;
    }

    int bytes = sk_entity_read(entity, NULL, read_len);
    if (bytes <= 0) {
        printf("buffer cannot read\n");
        return;
    }

    NetProc net_proc_msg = NET_PROC__INIT;
    net_proc_msg.data.len = bytes;
    net_proc_msg.data.data = fevbuff_rawget(evbuff);
    sk_sched_push(sched, fd, SK_PTO_NET_PROC, &net_proc_msg);

    // consume the evbuff
    fevbuff_pop(evbuff, bytes);
}

// send a destroy msg, so that the scheduler will destory it later
static
void _error(fev_state* fev, fev_buff* evbuff, void* arg)
{
    printf("evbuff destroy...\n");
    sk_entity_t* entity = arg;
    sk_sched_t* sched = sk_entity_sched(entity);
    int fd = fevbuff_get_fd(evbuff);

    NetDestroy destroy_msg = NET_DESTROY__INIT;
    sk_sched_push(sched, fd, SK_PTO_NET_DESTROY, &destroy_msg);
}

// register the new sock fd into eventloop
static
int _req(sk_sched_t* sched, sk_entity_t* entity, void* proto_msg)
{
    printf("event req\n");
    NetAccept* accept_msg = proto_msg;
    int client_fd = accept_msg->fd;

    fev_state* fev = sk_sched_eventloop(sched);
    fev_buff* evbuff = fevbuff_new(fev, client_fd, _read_cb, _error, entity);
    SK_ASSERT(evbuff);

    sk_net_entity_create(entity, evbuff);
    return 0;
}

sk_proto_t sk_pto_net_accept = {
    .priority = SK_PTO_PRI_8,
    .descriptor = &net_accept__descriptor,
    .run = _req
};
