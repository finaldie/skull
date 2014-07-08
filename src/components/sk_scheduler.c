#include <stdlib.h>

#include "api/sk_scheduler.h"

#define SK_SCHEDULER_PULL_NUM 1024

struct sk_scheduler_t {
    sk_io_t** io_tbl;
    int       running;
#if __WORDSIZE == 64
    int        padding;
#endif
};

sk_scheduler_t* sk_scheduler_create(sk_io_t** io_tbl)
{
    sk_scheduler_t* scheduler = malloc(sizeof(*scheduler));
    scheduler->running = 0;
    scheduler->io_tbl = io_tbl;

    // initialize IO systems
    for (int i = 0; io_tbl[i] != NULL; i++) {
        sk_io_t* sk_io = io_tbl[i];
        sk_io->init(sk_io->data);
    }

    return scheduler;
}

void sk_scheduler_destroy(sk_scheduler_t* scheduler)
{
    sk_io_t** io_tbl = scheduler->io_tbl;
    for (int i = 0; io_tbl[i] != NULL; i++) {
        sk_io_t* sk_io = io_tbl[i];
        sk_io->destroy(sk_io->data);
    }

    free(scheduler);
}

static
void _pull_and_run(sk_io_t* sk_io)
{
    sk_event_t events[SK_SCHEDULER_PULL_NUM];
    int nevents = sk_io->pull(sk_io->data, events, SK_SCHEDULER_PULL_NUM);

    int finished = 0;
    for (int i = 0; i < nevents && finished != nevents; i++) {
        sk_event_t* event = &events[i];

        if (event->end()) {
            event->destroy(event->data);
            finished++;
            continue;
        }

        int ret = event->run(event->data);
        if (ret) {
            event->destroy(sk_io->data);
            finished++;
            continue;
        }
    }
}

void sk_scheduler_start(sk_scheduler_t* scheduler)
{
    scheduler->running = 1;
    sk_io_t** io_tbl = scheduler->io_tbl;
    // open all the io systems
    for (int i = 0; io_tbl[i] != NULL; i++) {
        sk_io_t* sk_io = io_tbl[i];
        sk_io->open(sk_io->data);
    }

    do {
        for (int i = 0; io_tbl[i] != NULL; i++) {
            sk_io_t* sk_io = io_tbl[i];
            _pull_and_run(sk_io);
        }
    } while (scheduler->running);
}

void sk_scheduler_stop(sk_scheduler_t* scheduler)
{
    sk_io_t** io_tbl = scheduler->io_tbl;
    for (int i = 0; io_tbl[i] != NULL; i++) {
        sk_io_t* sk_io = io_tbl[i];
        sk_io->close(sk_io->data);
    }

    scheduler->running = 0;
}
