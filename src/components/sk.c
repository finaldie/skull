#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "fnet/fnet_core.h"
#include "fev/fev.h"
#include "fev/fev_listener.h"

#include "api/sk_types.h"
#include "api/sk_utils.h"
#include "api/sk_eventloop.h"
#include "api/sk_pto.h"
#include "api/sk.h"

// INTERNAL APIs
static
void _skull_setup_io(skull_core_t* core)
{
    skull_sched_t* main_sched = &core->main_sched;
    main_sched->sched = sk_sched_create();

    for (int i = 0; i < SKULL_WORKER_NUM; i++) {
        skull_sched_t* worker_sched = &core->worker_sched[i];
        worker_sched->sched = sk_sched_create();

        sk_sched_setup_bridge(main_sched->sched, worker_sched->sched);
    }
}

static
void _sk_accept(fev_state* fev, int fd, void* ud)
{
    skull_sched_t* sched = ud;

    NetAccept accept_msg = NET_ACCEPT__INIT;
    accept_msg.fd = fd;
    sk_sched_send(sched->sched, fd, SK_PTO_NET_ACCEPT, &accept_msg);
}

static
void _setup_listener(skull_sched_t* sched)
{
    int listen_fd = fnet_create_listen(NULL, 7758, 1024, 0);
    SK_ASSERT_MSG(listen_fd, "listen_fd = %d\n", listen_fd);

    fev_state* fev = sk_sched_eventloop(sched->sched);
    fev_listen_info* fli = fev_add_listener_byfd(fev, listen_fd,
                                                 _sk_accept, sched);
    SK_ASSERT(fli);
}

static
void _skull_setup_evloop(skull_core_t* core)
{
    // setup main evloop
    skull_sched_t* main_sched = &core->main_sched;
    _setup_listener(main_sched);
}

static
void* main_io_thread(void* arg)
{
    sk_print("main io thread started\n");
    sk_sched_t* sched = arg;
    sk_sched_start(sched);

    return 0;
}

static
void* worker_io_thread(void* arg)
{
    sk_print("worker io thread started\n");
    sk_sched_t* sched = arg;
    sk_sched_start(sched);

    return 0;
}

// APIs
void skull_init(skull_core_t* core)
{
    // 1. load config
    // 2. load modules
    // 3. init schedulers
    _skull_setup_io(core);
    _skull_setup_evloop(core);
}

void skull_start(skull_core_t* core)
{
    // 1. start the main io thread
    // 2. start the worker io threads
    sk_print("skull engine starting...\n");

    // 1.
    skull_sched_t* main_sched = &core->main_sched;
    int ret = pthread_create(&main_sched->io_thread, NULL,
                             main_io_thread, main_sched->sched);
    if (ret) {
        sk_print("create main io thread failed: %s\n", strerror(errno));
        exit(ret);
    }

    // 2.
    for (int i = 0; i < SKULL_WORKER_NUM; i++) {
        skull_sched_t* worker_sched = &core->worker_sched[i];
        ret = pthread_create(&worker_sched->io_thread, NULL,
                             worker_io_thread, worker_sched->sched);
        if (ret) {
            sk_print("create worker io thread failed: %s\n", strerror(errno));
            exit(ret);
        }
    }

    // 3. wait them quit
    for (int i = 0; i < SKULL_WORKER_NUM; i++) {
        pthread_join(core->worker_sched[i].io_thread, NULL);
    }
    pthread_join(main_sched->io_thread, NULL);
}

void skull_stop(skull_core_t* core)
{
    sk_print("skull engine stoping...\n");
    for (int i = 0; i < SKULL_WORKER_NUM; i++) {
        sk_sched_destroy(core->worker_sched[i].sched);
    }

    sk_sched_destroy(core->main_sched.sched);
}


