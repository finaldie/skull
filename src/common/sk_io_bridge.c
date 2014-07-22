#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <stdint.h>

#include "fev/fev.h"
#include "fmbuf/fmbuf.h"
#include "api/sk_assert.h"
#include "api/sk_event.h"
#include "api/sk_io_bridge.h"

struct sk_io_bridge_t {
    sk_io_t* src_io;
    sk_io_t* dst_io;
    void*    dst_evlp;
    fmbuf*   mq;
    int      evfd;

#if __WORDSIZE == 64
    int        padding;
#endif
};

// INTERNAL APIs

// copy the events from bridge mq
// NOTES: this copy action will be executed on the dst eventloop thread,
//        here we only can do fetch and copy, DO NOT PUSH events into it
static
void _copy_event(fev_state* fev, int fd, int mask, void* arg)
{
    sk_io_bridge_t* io_bridge = arg;
    uint64_t event_cnt = 0;

    if (eventfd_read(fd, &event_cnt)) {
        // some error occured
        return;
    }

    if (!event_cnt) {
        return;
    }

    int free_cnt = sk_io_free(io_bridge->dst_io, SK_IO_INPUT);
    int mq_cnt = fmbuf_used(io_bridge->mq) / SK_EVENT_SZ;
    int copy_cnt = mq_cnt > free_cnt ? free_cnt : mq_cnt;
    int copy_sz = SK_EVENT_SZ * copy_cnt;

    sk_event_t* events = malloc(copy_sz);
    int ret = fmbuf_pop(io_bridge->mq, events, copy_sz);
    SK_ASSERT(!ret);

    sk_io_push(io_bridge->dst_io, SK_IO_INPUT, events, copy_cnt);
    free(events);
}

static
void _setup_bridge(sk_io_bridge_t* io_bridge)
{
    int ret = fev_reg_event(io_bridge->dst_evlp, io_bridge->evfd, FEV_READ,
                            _copy_event, NULL, io_bridge);
    SK_ASSERT(!ret);
}

// APIs
sk_io_bridge_t* sk_io_bridge_create(sk_io_t* src_io, sk_io_t* dst_io,
                                    void* dst_evlp, int size)
{
    sk_io_bridge_t* io_bridge = malloc(sizeof(*io_bridge));
    io_bridge->src_io = src_io;
    io_bridge->dst_io = dst_io;
    io_bridge->dst_evlp = dst_evlp;
    io_bridge->mq = fmbuf_create(SK_EVENT_SZ * size);
    io_bridge->evfd = eventfd(0, EFD_NONBLOCK);

    _setup_bridge(io_bridge);

    return io_bridge;
}

void sk_io_bridge_destroy(sk_io_bridge_t* io_bridge)
{
    close(io_bridge->evfd);
    free(io_bridge);
}

// working flow:
// 1. try to copy all the events from src_io into io_bridge's mq
// 2. notify the dst eventloop how many events from io_bridge's mq
// 3. dst eventloop receive the notification, then fetch the events to dst io's
//    input mq
// 4. waiting for dst's scheduler to consume the events
void sk_io_bridge_deliver(sk_io_bridge_t* io_bridge)
{
    // calculate how many events can be transferred
    int event_cnt = sk_io_used(io_bridge->src_io, SK_IO_OUTPUT);
    if (event_cnt == 0) {
        return;
    }
    int free_slots = fmbuf_free(io_bridge->mq) / SK_EVENT_SZ;
    int transfer_cnt = free_slots > event_cnt ? event_cnt : free_slots;
    int transfer_sz = transfer_cnt * SK_EVENT_SZ;

    // copy events from src_io to io bridge's mq
    sk_event_t* transfer_events = malloc(transfer_sz);
    int nevents = sk_io_pull(io_bridge->src_io, SK_IO_OUTPUT,
                         transfer_events, transfer_cnt);
    SK_ASSERT(nevents == transfer_cnt);
    int ret = fmbuf_push(io_bridge->mq, transfer_events, transfer_sz);
    SK_ASSERT(!ret);
    free(transfer_events);

    // notify dst eventloop the data is ready
    eventfd_write(io_bridge->evfd, event_cnt);
}
