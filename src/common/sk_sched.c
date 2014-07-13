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
    sk_io_t*        io_tbl[SK_SCHED_MAX_IO];
    sk_io_bridge_t* io_bridge_tbl[SK_SCHED_MAX_IO_BRIDGE];
    sk_sched_opt_t  opt;

    int        io_size;
    int        io_bridge_size;
};

// APIs
sk_sched_t* sk_sched_create(sk_sched_opt_t opt)
{
    sk_sched_t* sched = malloc(sizeof(*sched));
    memset(sched, 0, sizeof(*sched));

    sched->io_size = 0;
    sched->io_bridge_size = 0;
    sched->opt = opt;

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

void sk_sched_run(sk_sched_t* sched)
{
    sched->opt.schedule(sched, sched->io_tbl, sched->io_bridge_tbl);
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
