#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "api/sk_eventloop.h"
#include "api/sk_io.h"
#include "api/sk_sched.h"

#define SKULL_WORKER_NUM 2

typedef struct skull_sched_t {
    pthread_t   io_thread;
    sk_sched_t* sched;
    void*       evlp;
} skull_sched_t;

typedef struct skull_core_t {
    skull_sched_t main_sched;
    skull_sched_t worker_sched[SKULL_WORKER_NUM];
} skull_core_t;

static
void _skull_setup_io(skull_core_t* core)
{
    skull_sched_t* main_sched = &core->main_sched;
    main_sched->evlp = sk_eventloop_create();
    main_sched->sched = sk_main_sched_create(main_sched->evlp,
                                             SK_SCHED_STRATEGY_THROUGHPUT);

    for (int i = 0; i < SKULL_WORKER_NUM; i++) {
        skull_sched_t* worker_sched = &core->worker_sched[i];

        worker_sched->evlp = sk_eventloop_create();
        worker_sched->sched = sk_worker_sched_create(worker_sched->evlp,
                                            SK_SCHED_STRATEGY_THROUGHPUT);
        sk_io_t* main_accept_io = sk_io_create(1024);
        sk_io_t* worker_accept_io = sk_io_create(1024);
        sk_io_t* worker_net_io = sk_io_create(65535);

        sk_sched_reg_io(main_sched->sched, SK_IO_NET_ACCEPT, main_accept_io);
        sk_sched_reg_io(worker_sched->sched, SK_IO_NET_ACCEPT, worker_accept_io);
        sk_sched_reg_io(worker_sched->sched, SK_IO_NET_SOCK, worker_net_io);

        // set up the double linked io bridge for accept io
        sk_io_bridge_t* worker_io_bridge = sk_io_bridge_create(
                                                    worker_accept_io,
                                                    main_accept_io,
                                                    main_sched->evlp,
                                                    1024);
        sk_io_bridge_t* main_io_bridge = sk_io_bridge_create(
                                                    main_accept_io,
                                                    worker_accept_io,
                                                    worker_sched->evlp,
                                                    1024);

        sk_sched_reg_io_bridge(worker_sched->sched, worker_io_bridge);
        sk_sched_reg_io_bridge(main_sched->sched, main_io_bridge);
    }
}

static
void skull_init(skull_core_t* core)
{
    // 1. load config
    // 2. load modules
    // 3. init schedulers
    _skull_setup_io(core);
}

static
void* main_io_thread(void* arg)
{
    printf("main io thread started\n");
    sk_sched_t* sched = arg;
    sk_sched_start(sched);

    return 0;
}

static
void* worker_io_thread(void* arg)
{
    printf("worker io thread started\n");
    sk_sched_t* sched = arg;
    sk_sched_start(sched);

    return 0;
}

static
void skull_start(skull_core_t* core)
{
    // 1. start the main io thread
    // 2. start the worker io threads
    printf("skull engine starting...\n");

    // 1.
    skull_sched_t* main_sched = &core->main_sched;
    int ret = pthread_create(&main_sched->io_thread, NULL,
                             main_io_thread, main_sched->sched);
    if (ret) {
        printf("create main io thread failed: %s\n", strerror(errno));
        exit(ret);
    }

    // 2.
    for (int i = 0; i < SKULL_WORKER_NUM; i++) {
        skull_sched_t* worker_sched = &core->worker_sched[i];
        ret = pthread_create(&worker_sched->io_thread, NULL,
                             worker_io_thread, worker_sched->sched);
        if (ret) {
            printf("create worker io thread failed: %s\n", strerror(errno));
            exit(ret);
        }
    }

    // 3. wait them quit
    for (int i = 0; i < SKULL_WORKER_NUM; i++) {
        pthread_join(core->worker_sched[i].io_thread, NULL);
    }
    pthread_join(main_sched->io_thread, NULL);
}

static
void skull_stop(skull_core_t* core)
{
    printf("skull engine stoping...\n");
    for (int i = 0; i < SKULL_WORKER_NUM; i++) {
        sk_sched_destroy(core->worker_sched[i].sched);
        sk_eventloop_destroy(core->worker_sched[i].evlp);
    }

    sk_sched_destroy(core->main_sched.sched);
    sk_eventloop_destroy(core->main_sched.evlp);
}

int main(int argc, char** argv)
{
    printf("hello skull engine\n");

    skull_core_t core;
    memset(&core, 0, sizeof(core));
    skull_init(&core);
    skull_start(&core);
    skull_stop(&core);
    return 0;
}
