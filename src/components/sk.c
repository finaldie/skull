#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>

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
#include "api/sk_env.h"
#include "api/sk.h"

// INTERNAL APIs

static
sk_thread_env_t* _skull_thread_env_create(skull_sched_t* skull_sched,
                                          flist* workflows)
{
    sk_thread_env_t* thread_env = malloc(sizeof(*thread_env));
    thread_env->sched = skull_sched->sched;
    thread_env->entity_mgr = skull_sched->entity_mgr;
    thread_env->workflows = workflows;
    thread_env->evlp = skull_sched->evlp;

    return thread_env;
}

static
void _skull_setup_schedulers(skull_core_t* core)
{
    sk_config_t* config = core->config;

    // create main scheduler
    skull_sched_t* main_sched = &core->main_sched;
    main_sched->evlp = sk_eventloop_create();
    main_sched->entity_mgr = sk_entity_mgr_create(65535);
    main_sched->sched = sk_sched_create(main_sched->evlp,
                                        main_sched->entity_mgr);

    // create worker schedulers
    core->worker_sched = calloc(config->threads, sizeof(skull_sched_t));

    for (int i = 0; i < config->threads; i++) {
        skull_sched_t* worker_sched = &core->worker_sched[i];

        worker_sched->evlp = sk_eventloop_create();
        worker_sched->entity_mgr = sk_entity_mgr_create(65535);
        worker_sched->sched = sk_sched_create(worker_sched->evlp,
                                              worker_sched->entity_mgr);

        sk_sched_setup_bridge(main_sched->sched, worker_sched->sched);
    }
}

// this is running on the main scheduler io thread
// NOTES: The trigger is responsible for creating a base entity object
static
void _sk_accept(fev_state* fev, int fd, void* ud)
{
    sk_workflow_t* workflow = ud;
    sk_sched_t* sched = SK_THREAD_ENV_SCHED;

    sk_entity_t* entity = sk_entity_create(workflow);
    sk_print("create a new entity(%d)\n", fd);

    NetAccept accept_msg = NET_ACCEPT__INIT;
    accept_msg.fd = fd;
    sk_sched_send(sched, entity, NULL, SK_PTO_NET_ACCEPT, &accept_msg);
}

static
void _setup_net_trigger(skull_sched_t* sched, sk_workflow_t* workflow)
{
    fev_state* fev = sched->evlp;
    int listen_fd = workflow->trigger.network.listen_fd;
    fev_listen_info* fli = fev_add_listener_byfd(fev, listen_fd,
                                                 _sk_accept, workflow);
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
        _setup_net_trigger(main_sched, workflow);
    }
}

static
void _skull_setup_workflow(skull_core_t* core)
{
    sk_config_t* config = core->config;
    core->workflows = flist_create();
    core->unique_modules = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
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
            // 3. store it to the unique module list

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

            // 3.
            fhash_str_set(core->unique_modules, module_name, module);
        }

        // store this workflow to skull_core::workflows
        int ret = flist_push(core->workflows, workflow);
        SK_ASSERT(!ret);

        // set up trigger
        sk_print("set up trigger...\n");
        _skull_setup_trigger(core, workflow);
    }
}

static
void* main_io_thread(void* arg)
{
    sk_print("main io thread started\n");
    sk_thread_env_t* thread_env = arg;
    sk_thread_env_set(thread_env);

    sk_sched_t* sched = thread_env->sched;
    sk_sched_start(sched);
    return 0;
}

static
void* worker_io_thread(void* arg)
{
    sk_print("worker io thread started\n");
    sk_thread_env_t* thread_env = arg;
    sk_sched_t* sched = thread_env->sched;
    sk_thread_env_set(thread_env);

    sk_sched_start(sched);
    return 0;
}

static
void _skull_module_init(skull_core_t* core)
{
    fhash_str_iter iter = fhash_str_iter_new(core->unique_modules);
    sk_module_t* module = NULL;

    while ((module = fhash_str_next(&iter))) {
        sk_print("module init...\n");
        module->sk_module_init();
    }
}

// APIs
void skull_init(skull_core_t* core)
{
    // 1. prepare the thread env
    sk_thread_env_init();

    // 2. load config
    core->config = sk_config_create(core->cmd_args.config_location);

    // 3. change working-dir to the config dir
    char* raw_path = strdup(core->cmd_args.config_location);
    const char* config_dir = dirname(raw_path);
    int ret = chdir(config_dir);
    SK_ASSERT_MSG(!ret, "change working dir to %s failed, errno:%s\n",
                  config_dir, strerror(errno));
    free(raw_path);
    core->working_dir = getcwd(NULL, 0);
    sk_print("current working dir: %s\n", core->working_dir);

    // 4. init schedulers
    _skull_setup_schedulers(core);

    // 5. load working flows
    _skull_setup_workflow(core);
}

void skull_start(skull_core_t* core)
{
    // 1. module init
    _skull_module_init(core);

    sk_config_t* config = core->config;

    // 2. start the main io thread
    // 3. start the worker io threads
    sk_print("skull engine starting...\n");

    // 2.
    skull_sched_t* main_sched = &core->main_sched;
    // this *main_thread_env* will be deleted when thread exit
    sk_thread_env_t* main_thread_env = _skull_thread_env_create(main_sched,
                                                               core->workflows);
    int ret = pthread_create(&main_sched->io_thread, NULL,
                             main_io_thread, main_thread_env);
    if (ret) {
        sk_print("create main io thread failed: %s\n", strerror(errno));
        exit(ret);
    }

    // 3.
    for (int i = 0; i < config->threads; i++) {
        skull_sched_t* worker_sched = &core->worker_sched[i];
        // this *worker_thread_env* will be deleted when thread exit
        sk_thread_env_t* worker_thread_env = _skull_thread_env_create(
                                                               main_sched,
                                                               core->workflows);
        ret = pthread_create(&worker_sched->io_thread, NULL,
                             worker_io_thread, worker_thread_env);
        if (ret) {
            sk_print("create worker io thread failed: %s\n", strerror(errno));
            exit(ret);
        }
    }

    // 4. wait them quit
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
    fhash_str_delete(core->unique_modules);
}


