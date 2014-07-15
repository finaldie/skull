#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "api/sk_assert.h"
#include "api/sk_eventloop.h"
#include "api/sk_io_bridge.h"
#include "api/sk_sched.h"

#define SK_SCHED_PULL_NUM   1024

#define SK_SCHED_MAX_IO        10
#define SK_SCHED_MAX_IO_BRIDGE 10

struct sk_sched_t {
    void* evlp;
    sk_sched_opt_t  opt;
    sk_sched_io_t   io_tbl[SK_SCHED_MAX_IO];
    sk_io_bridge_t* io_bridge_tbl[SK_SCHED_MAX_IO_BRIDGE];
    int        io_size;
    int        io_bridge_size;
    uint32_t   running:1;
    uint32_t   reserved:31;
#if __WORDSIZE == 64
    int         padding;
#endif
};

// INTERNAL APIs

// NOTE: need to add a selecting strategy
static
sk_sched_io_t* _get_io(sk_sched_t* sched, int deliver_to)
{
    for (int i = 0; sched->io_tbl[i].io != NULL; i++) {
        sk_sched_io_t* io = &sched->io_tbl[i];
        if (io->type == deliver_to) {
            return io;
        }
    }

    return NULL;
}

static
int _run_event(sk_sched_t* sched, sk_io_t* io, sk_event_t* event)
{
    uint32_t pto_id = event->pto_id;
    sk_event_opt_t* event_opt = sched->opt.pto_tbl[pto_id];
    printf("pto_id = %u\n", pto_id);
    SK_ASSERT(event_opt);

    if (event->type == SK_EV_REQ) {
        event_opt->req(sched, event);

        if (event_opt->resp) {
            event->type = SK_EV_RESP;
            SK_ASSERT(!sk_io_push(io, SK_IO_OUTPUT, event, 1));
        }
    } else {
        if (event_opt->resp) {
            event_opt->resp(sched, event);
        }
    }

    if (event_opt->end(sched, event)) {
        if (event_opt->destroy) {
            event_opt->destroy(sched, event);
        }
    } else {
        SK_ASSERT(!sk_io_push(io, SK_IO_INPUT, event, 1));
    }
    return 0;
}

static
void _pull_and_run(sk_sched_t* sched, sk_io_t* sk_io)
{
    sk_event_t events[SK_SCHED_PULL_NUM];
    int nevents = sk_io_pull(sk_io, SK_IO_INPUT, events, SK_SCHED_PULL_NUM);

    for (int i = 0; i < nevents; i++) {
        sk_event_t* event = &events[i];
        _run_event(sched, sk_io, event);
    }
}

// APIs
sk_sched_t* sk_sched_create(void* evlp, sk_sched_opt_t opt)
{
    sk_sched_t* sched = malloc(sizeof(*sched));
    memset(sched, 0, sizeof(*sched));
    sched->evlp = evlp;
    sched->opt = opt;
    sched->io_size = 0;
    sched->io_bridge_size = 0;
    sched->running = 0;

    return sched;
}

void sk_sched_destroy(sk_sched_t* sched)
{
    sk_sched_io_t* io_tbl = sched->io_tbl;
    for (int i = 0; i < sched->io_size; i++) {
        sk_io_t* sk_io = io_tbl[i].io;
        sk_io_destroy(sk_io);
    }

    free(sched);
}

void sk_sched_start(sk_sched_t* sched)
{
    sched->running = 1;

    do {
        // pull all io events and convert them to sched events
        int nprocessed = sk_eventloop_dispatch(sched->evlp, 1000);
        if (nprocessed <= 0 ) {
            continue;
        }

        // execuate all io events
        for (int i = 0; sched->io_tbl[i].io != NULL; i++) {
            sk_sched_io_t* io = &sched->io_tbl[i];
            _pull_and_run(sched, io->io);
        }

        // execuate all io bridge
        for (int i = 0; sched->io_bridge_tbl[i] != NULL; i++) {
            sk_io_bridge_t* io_bridge = sched->io_bridge_tbl[i];
            sk_io_bridge_deliver(io_bridge);
        }
    } while (sched->running);
}

void sk_sched_stop(sk_sched_t* sched)
{
    sched->running = 0;
}

// the io must be register one by one
int sk_sched_reg_io(sk_sched_t* sched, int io_type, sk_io_t* io)
{
    if (sched->io_size == SK_SCHED_MAX_IO) {
        return 1;
    }

    if (!io) {
        return 1;
    }

    sched->io_tbl[sched->io_size].type = io_type;
    sched->io_tbl[sched->io_size].status = SK_IO_STAT_READY;
    sched->io_tbl[sched->io_size].io = io;
    sched->io_size++;
    return 0;
}

int sk_sched_reg_io_bridge(sk_sched_t* sched,
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

// push a event into scheduler, and the scheduler will route it to the specific
// sk_io
int sk_sched_push(sk_sched_t* sched, sk_event_t* event)
{
    int deliver_to = event->deliver;
    sk_sched_io_t* io = _get_io(sched, deliver_to);
    SK_ASSERT(io);

    if (io->status == SK_IO_STAT_PAUSE) {
        return 1;
    }

    if (sk_io_free(io->io, event->ev_type)) {
        SK_ASSERT(!sk_io_push(io->io, event->ev_type, event, 1));
    }
    return 0;
}

void* sk_sched_get_eventloop(sk_sched_t* sched)
{
    return sched->evlp;
}
