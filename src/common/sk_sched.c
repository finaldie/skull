#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "api/sk_assert.h"
#include "api/sk_eventloop.h"
#include "api/sk_io_bridge.h"
#include "api/sk_sched.h"

#define SK_SCHED_PULL_NUM   1024

#define SK_SCHED_MAX_IO        64
#define SK_SCHED_MAX_IO_BRIDGE 64

#define SK_IO_STAT_READY 0
#define SK_IO_STAT_PAUSE 1

typedef struct sk_sched_io_t {
    int type;
    int status;
    sk_io_t* io;
} sk_sched_io_t;

struct sk_sched_t {
    void* evlp;
    sk_entity_mgr_t*  entity_mgr;
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

static
void _check_ptos(sk_event_opt_t** pto_tbl)
{
    if (!pto_tbl) {
        return;
    }

    for (int i = 0; pto_tbl[i] != NULL; i++) {
        sk_event_opt_t* pto = pto_tbl[i];
        SK_ASSERT(pto->req);
    }
}

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
void _run_req_event(sk_sched_t* sched, sk_io_t* io, sk_event_t* event,
                    sk_event_opt_t* pto)
{
    pto->req(sched, event);

    if (pto->req_end && !pto->req_end(sched, event)) {
        sk_io_push(io, SK_IO_INPUT, event, 1);
        return;
    }

    if (pto->resp) {
        event->type = SK_EV_RESP;
        sk_io_push(io, SK_IO_OUTPUT, event, 1);
        return;
    }

    if (pto->destroy) {
        pto->destroy(sched, event);
    }
}

static
void _run_resp_event(sk_sched_t* sched, sk_io_t* io, sk_event_t* event,
                     sk_event_opt_t* pto)
{
    if (pto->resp) {
        pto->resp(sched, event);
    }

    if (pto->resp_end && !pto->resp_end(sched, event)) {
        sk_io_push(io, SK_IO_OUTPUT, event, 1);
        return;
    }

    if (pto->destroy) {
        pto->destroy(sched, event);
    }
}

static
int _run_event(sk_sched_t* sched, sk_io_t* io, sk_event_t* event)
{
    uint32_t pto_id = event->pto_id;
    sk_event_opt_t* event_opt = sched->opt.pto_tbl[pto_id];
    printf("pto_id = %u\n", pto_id);
    SK_ASSERT(event_opt);

    if (event->type == SK_EV_REQ) {
        _run_req_event(sched, io, event, event_opt);
    } else {
        _run_resp_event(sched, io, event, event_opt);
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
    sched->entity_mgr = sk_entity_mgr_create(sched, 65535);
    sched->opt = opt;
    sched->io_size = 0;
    sched->io_bridge_size = 0;
    sched->running = 0;

    _check_ptos(opt.pto_tbl);
    return sched;
}

void sk_sched_destroy(sk_sched_t* sched)
{
    sk_sched_io_t* io_tbl = sched->io_tbl;
    for (int i = 0; i < sched->io_size; i++) {
        sk_io_t* sk_io = io_tbl[i].io;
        sk_io_destroy(sk_io);
    }

    sk_entity_mgr_destroy(sched->entity_mgr);
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

        // clean dead entities
        sk_entity_mgr_clean_dead(sched->entity_mgr);
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
    // the event must always initialized with SK_PTO_REQ
    event->type = SK_EV_REQ;

    int deliver_to = event->deliver;
    sk_sched_io_t* io = _get_io(sched, deliver_to);
    SK_ASSERT(io);

    if (io->status == SK_IO_STAT_PAUSE) {
        return 1;
    }

    int io_type = event->ev_type == SK_EV_INCOMING ? SK_IO_INPUT : SK_IO_OUTPUT;
    if (sk_io_free(io->io, io_type)) {
        sk_io_push(io->io, io_type, event, 1);
    }
    return 0;
}

void* sk_sched_get_eventloop(sk_sched_t* sched)
{
    return sched->evlp;
}

sk_entity_mgr_t* sk_sched_entity_mgr(sk_sched_t* sched)
{
    return sched->entity_mgr;
}
