#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fhash/fhash.h"
#include "api/sk_eventloop.h"
#include "api/sk_io_bridge.h"
#include "api/sk_scheduler.h"

#define SK_SCHED_PULL_NUM   1024

#define SK_SCHED_MAX_IO        10
#define SK_SCHED_MAX_IO_BRIDGE 10

#define SK_SCHED_MAX_STRATEGY  2

typedef void (*sk_sched_strategy) (sk_sched_t* sched);

struct sk_sched_t {
    void*      evlp;
    sk_io_t*   io_tbl[SK_SCHED_MAX_IO];
    sk_io_bridge_t* io_bridge_tbl[SK_SCHED_MAX_IO_BRIDGE];
    sk_sched_strategy stragegy_tbl[SK_SCHED_MAX_STRATEGY];

    int        io_size;
    int        io_bridge_size;
    uint32_t   running  :1;
    uint32_t   strategy :2;
    uint32_t   reserved :29;

#if __WORDSIZE == 64
    int        padding;
#endif
};

// INTERNAL API
static
void _pull_and_run(sk_io_t* sk_io)
{
    sk_event_t events[SK_SCHED_PULL_NUM];
    int nevents = sk_io_pull(sk_io, SK_IO_INPUT, events, SK_SCHED_PULL_NUM);

    for (int i = 0; i < nevents; i++) {
        sk_event_t* event = &events[i];

        if (event->end()) {
            event->cb(event->data);
            event->destroy(event->data);
            continue;
        }

        int ret = event->run(event->data);
        if (ret) {
            event->destroy(event->data);
            continue;
        }
    }
}

static
void _process_io_bridge(sk_io_bridge_t* io_bridge)
{
    sk_io_bridge_deliver(io_bridge);
}

static
void _sched_throughput(sk_sched_t* sched)
{
    sk_io_t** io_tbl = sched->io_tbl;
    sk_io_bridge_t** io_bridge_tbl = sched->io_bridge_tbl;

    // execuate all io events
    for (int i = 0; i < sched->io_size; i++) {
        sk_io_t* sk_io = io_tbl[i];
        _pull_and_run(sk_io);
    }

    // execuate all io bridge
    for (int i = 0; i < sched->io_bridge_size; i++) {
        sk_io_bridge_t* io_bridge = io_bridge_tbl[i];
        _process_io_bridge(io_bridge);
    }
}

static
void _sched_latency(sk_sched_t* sched)
{

}

// APIs
sk_sched_t* sk_sched_create(void* evlp, int strategy)
{
    sk_sched_t* sched = malloc(sizeof(*sched));
    memset(sched, 0, sizeof(*sched));

    sched->evlp = evlp;
    sched->stragegy_tbl[SK_SCHED_STRATEGY_THROUGHPUT] = _sched_throughput;
    sched->stragegy_tbl[SK_SCHED_STRATEGY_LATENCY] = _sched_latency;
    sched->io_size = 0;
    sched->io_bridge_size = 0;
    sched->running = 0;
    sched->strategy = strategy;

    return sched;
}

void sk_sched_destroy(sk_sched_t* sched)
{
    sk_io_t** io_tbl = sched->io_tbl;
    for (int i = 0; i < sched->io_size; i++) {
        sk_io_t* sk_io = io_tbl[i];
        sk_io_destroy(sk_io);
    }

    free(sched);
}

void sk_sched_start(sk_sched_t* sched)
{
    sched->running = 1;

    // event loop start
    do {
        // pull all io events and convert them to sched events
        int nprocessed = sk_eventloop_dispatch(sched->evlp, 10);
        if (nprocessed <= 0 ) {
            continue;
        }

        // run events by strategy
        sched->stragegy_tbl[sched->strategy](sched);
    } while (sched->running);
}

void sk_sched_stop(sk_sched_t* sched)
{
    sk_io_t** io_tbl = sched->io_tbl;
    for (int i = 0; i < sched->io_size; i++) {
        sk_io_t* sk_io = io_tbl[i];
        sk_io_destroy(sk_io);
    }

    sched->running = 0;
}

int sk_sched_register_io(sk_sched_t* sched, sk_io_t* io)
{
    if (sched->io_size == SK_SCHED_MAX_IO) {
        return 1;
    }

    if (!io) {
        return 1;
    }

    sched->io_tbl[sched->io_size++] = io;
    return 0;
}

int sk_sched_register_io_bridge(sk_sched_t* sched,
                                sk_io_bridge_t* io_bridge)
{
    if (sched->io_bridge_size == SK_SCHED_MAX_IO_BRIDGE) {
        return 1;
    }

    if (!io_bridge) {
        return 1;
    }

    sched->io_bridge_tbl[sched->io_bridge_size++] = io_bridge;
    return 0;
}
