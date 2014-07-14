#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "api/sk_eventloop.h"
#include "api/sk_io_bridge.h"
#include "api/sk_sched.h"

#define SK_SCHED_PULL_NUM   1024

#define SK_SCHED_MAX_IO        10
#define SK_SCHED_MAX_IO_BRIDGE 10

struct sk_sched_t {
    void* evlp;
    sk_sched_opt_t  opt;
    sk_io_t*        io_tbl[SK_SCHED_MAX_IO];
    sk_io_bridge_t* io_bridge_tbl[SK_SCHED_MAX_IO_BRIDGE];
    int        io_size;
    int        io_bridge_size;
    uint32_t   running:1;
    uint32_t   reserved:31;
#if __WORDSIZE == 64
    int         padding;
#endif
};

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

    do {
        // pull all io events and convert them to sched events
        int nprocessed = sk_eventloop_dispatch(sched->evlp, 1000);
        if (nprocessed <= 0 ) {
            continue;
        }

        sched->opt.schedule(sched, sched->io_tbl, sched->io_bridge_tbl);
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

    sched->io_tbl[io_type] = io;
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
