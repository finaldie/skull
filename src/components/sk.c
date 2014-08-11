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
#include "api/sk_config.h"
#include "api/sk_module.h"
#include "api/sk_workflow.h"
#include "api/sk_loader.h"
#include "api/sk.h"

// INTERNAL APIs

static
void _skull_setup_schedulers(skull_core_t* core)
{
    sk_config_t* config = core->config;
    skull_sched_t* main_sched = &core->main_sched;
    main_sched->sched = sk_sched_create();
    core->worker_sched = calloc(config->threads, sizeof(skull_sched_t));

    for (int i = 0; i < config->threads; i++) {
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
void _setup_net_trigger(skull_sched_t* sched, int listen_fd)
{
    fev_state* fev = sk_sched_eventloop(sched->sched);
    fev_listen_info* fli = fev_add_listener_byfd(fev, listen_fd,
                                                 _sk_accept, sched);
    SK_ASSERT(fli);
}


static
void _skull_setup_trigger(skull_core_t* core, sk_workflow_t* workflow)
{
    if (workflow->type == SK_WORKFLOW_MAIN) {
        sk_print("no need to set up trigger\n");
        return;
    }

    if (workflow->type == SK_WORKFLOW_TRIGGER) {
        // setup main evloop
        skull_sched_t* main_sched = &core->main_sched;
        _setup_net_trigger(main_sched, workflow->trigger.network.listen_fd);
    }
}

static
void _skull_setup_workflow(skull_core_t* core)
{
    sk_config_t* config = core->config;
    core->workflows = flist_create();
    flist_iter iter = flist_new_iter(config->workflows);
    sk_workflow_cfg_t* workflow_cfg = NULL;

    while ((workflow_cfg = flist_each(&iter))) {
        sk_print("setup workflow, detail info:\n");

        // set up the type and concurrent
        sk_workflow_t* workflow = sk_workflow_create(workflow_cfg->concurrent,
                                                     workflow_cfg->port);

        // set up modules
        flist_iter name_iter = flist_new_iter(workflow_cfg->modules);
        char* module_name = NULL;
        while ((module_name = flist_each(&name_iter))) {
            sk_print("loading module: %s\n", module_name);
            // 1. load the module
            // 2. add the module into workflow

            // 1.
            // TODO: use current folder as the default module location
            sk_module_t* module = sk_module_load(module_name);
            if (module) {
                sk_print("load module [%s] successful\n", module_name);
            } else {
                sk_print("load module [%s] failed\n", module_name);
            }

            // 2.
            int ret = sk_workflow_add_module(workflow, module);
            SK_ASSERT_MSG(!ret, "add module {%s} to workflow failed\n",
                          module_name);
        }

        // store this workflow to skull_core::workflows
        int ret = flist_push(core->workflows, workflow);
        SK_ASSERT(!ret);

        // set up trigger
        _skull_setup_trigger(core, workflow);
    }
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
    core->config = sk_config_create(core->cmd_args.config_location);

    // 2. init schedulers
    _skull_setup_schedulers(core);

    // 3. load working flows
    _skull_setup_workflow(core);
}

void skull_start(skull_core_t* core)
{
    sk_config_t* config = core->config;

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
    for (int i = 0; i < config->threads; i++) {
        skull_sched_t* worker_sched = &core->worker_sched[i];
        ret = pthread_create(&worker_sched->io_thread, NULL,
                             worker_io_thread, worker_sched->sched);
        if (ret) {
            sk_print("create worker io thread failed: %s\n", strerror(errno));
            exit(ret);
        }
    }

    // 3. wait them quit
    for (int i = 0; i < config->threads; i++) {
        pthread_join(core->worker_sched[i].io_thread, NULL);
    }
    pthread_join(main_sched->io_thread, NULL);
}

void skull_stop(skull_core_t* core)
{
    sk_print("skull engine stoping...\n");
    for (int i = 0; i < core->config->threads; i++) {
        sk_sched_destroy(core->worker_sched[i].sched);
    }

    free(core->worker_sched);
    sk_sched_destroy(core->main_sched.sched);
    sk_config_destroy(core->config);
    flist_delete(core->workflows);
}


