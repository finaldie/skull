#include <stdlib.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include <stdint.h>

#include "fev/fev.h"
#include "api/sk_assert.h"
#include "api/sk_event.h"
#include "api/sk_io_bridge.h"

struct sk_io_bridge_t {
    sk_io_t* src_io;
    sk_io_t* dst_io;
    void*    dst_evlp;
    int      evfd;

#if __WORDSIZE == 64
    int        padding;
#endif
};

// INTERNAL APIs
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

    sk_event_t* events = malloc(sizeof(sk_event_t) * event_cnt);
    int nevents = sk_io_pull(io_bridge->src_io, SK_IO_INPUT, events, event_cnt);
    SK_ASSERT(nevents == (int)event_cnt);

    int ret = sk_io_push(io_bridge->dst_io, SK_IO_OUTPUT, events, nevents);
    SK_ASSERT(!ret);
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
                                    void* dst_evlp)
{
    sk_io_bridge_t* io_bridge = malloc(sizeof(*io_bridge));
    io_bridge->src_io = src_io;
    io_bridge->dst_io = dst_io;
    io_bridge->dst_evlp = dst_evlp;
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
// 1. notify the dst eventloop how many events from src io's output mq
// 2. dst eventloop receive the notification, then fetch the events to dst io's
//    input mq
// 3. waiting for dst's scheduler to consume the events
void sk_io_bridge_deliver(sk_io_bridge_t* io_bridge)
{
    int event_cnt = sk_io_size(io_bridge->src_io, SK_IO_OUTPUT);
    eventfd_write(io_bridge->evfd, event_cnt);
}
