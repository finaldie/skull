#include <stdlib.h>

#include "api/sk_assert.h"
#include "api/sk_sched.h"

#define SK_SCHED_PULL_NUM   1024

// INTERNAL APIs
static
void _pull_and_run(sk_io_t* sk_io)
{
    sk_event_t events[SK_SCHED_PULL_NUM];
    int nevents = sk_io_pull(sk_io, SK_IO_INPUT, events, SK_SCHED_PULL_NUM);

    for (int i = 0; i < nevents; i++) {
        sk_event_t* event = &events[i];
        sk_event_run(event);
    }
}

static
void _sched_throughput(sk_sched_t* sched,
                       sk_sched_io_t* io_tbl,
                       sk_io_bridge_t** io_bridge_tbl)
{
    // execuate all io events
    for (int i = 0; io_tbl[i].io != NULL; i++) {
        sk_sched_io_t* io = &io_tbl[i];
        _pull_and_run(io->io);
    }

    // execuate all io bridge
    for (int i = 0; io_bridge_tbl[i] != NULL; i++) {
        sk_io_bridge_t* io_bridge = io_bridge_tbl[i];
        sk_io_bridge_deliver(io_bridge);
    }
}

// APIs
sk_sched_t* sk_main_sched_create(void* evlp, int strategy)
{
    sk_sched_opt_t opt;
    if (strategy == SK_SCHED_STRATEGY_THROUGHPUT) {
        opt.schedule = _sched_throughput;
    } else {
        return NULL;
    }

    return sk_sched_create(evlp, opt);
}
