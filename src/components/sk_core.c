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
#include "api/sk_const.h"
#include "api/sk_env.h"
#include "api/sk_log.h"
#include "api/sk_log_tpl.h"
#include "api/sk_core.h"

// INTERNAL APIs

static
void _sk_setup_engines(sk_core_t* core)
{
    sk_config_t* config = core->config;

    // create main scheduler
    core->master = sk_engine_create();

    // create worker schedulers
    core->workers = calloc((size_t)config->threads, sizeof(sk_engine_t*));

    for (int i = 0; i < config->threads; i++) {
        core->workers[i] = sk_engine_create();
        SK_LOG_INFO(core->logger, "worker scheduler [%d] init successfully", i);

        sk_engine_link(core->master, core->workers[i]);
        SK_LOG_INFO(core->logger, "io bridge [%d] init successfully", i);
    }

    SK_LOG_INFO(core->logger, "skull schedulers init successfully");
}

// this is running on the main scheduler io thread
// NOTES: The trigger is responsible for creating a base entity object
static
void _sk_accept(fev_state* fev, int fd, void* ud)
{
    sk_workflow_t* workflow = ud;
    sk_sched_t* sched = SK_ENV_SCHED;

    sk_entity_t* entity = sk_entity_create(workflow);
    sk_print("create a new entity(%d)\n", fd);

    NetAccept accept_msg = NET_ACCEPT__INIT;
    accept_msg.fd = fd;
    sk_sched_send(sched, entity, NULL, SK_PTO_NET_ACCEPT, &accept_msg);
}

static
void _sk_setup_net_trigger(sk_engine_t* sched, sk_workflow_t* workflow)
{
    fev_state* fev = sched->evlp;
    int listen_fd = workflow->trigger.network.listen_fd;
    fev_listen_info* fli = fev_add_listener_byfd(fev, listen_fd,
                                                 _sk_accept, workflow);
    SK_ASSERT(fli);
}

static
void _sk_setup_trigger(sk_core_t* core, sk_workflow_t* workflow)
{
    if (workflow->type == SK_WORKFLOW_MAIN) {
        sk_print("no need to set up trigger\n");
        return;
    }

    if (workflow->type == SK_WORKFLOW_TRIGGER) {
        // setup main evloop
        sk_engine_t* master = core->master;
        _sk_setup_net_trigger(master, workflow);
    }

    SK_LOG_INFO(core->logger, "set up triggers successfully");
}

static
void _sk_setup_workflow(sk_core_t* core)
{
    sk_config_t* config = core->config;
    core->workflows = flist_create();
    core->unique_modules = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    flist_iter iter = flist_new_iter(config->workflows);
    sk_workflow_cfg_t* workflow_cfg = NULL;

    while ((workflow_cfg = flist_each(&iter))) {
        sk_print("setup workflow, detail info:\n");
        SK_LOG_INFO(core->logger, "setup one workflow...");

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
                SK_LOG_INFO(core->logger, "load module [%s] successful",
                          module_name);
            } else {
                sk_print("load module [%s] failed\n", module_name);
                SK_LOG_INFO(core->logger, "load module [%s] failed",
                          module_name);
            }

            // 2.
            int ret = sk_workflow_add_module(workflow, module);
            SK_ASSERT_MSG(!ret, "add module {%s} to workflow failed\n",
                          module_name);

            // 3.
            fhash_str_set(core->unique_modules, module_name, module);
        }

        // store this workflow to sk_core::workflows
        int ret = flist_push(core->workflows, workflow);
        SK_ASSERT(!ret);
        SK_LOG_INFO(core->logger, "setup one workflow successfully");

        // set up trigger
        sk_print("set up trigger...\n");
        _sk_setup_trigger(core, workflow);
    }
}

static
void* main_io_thread(void* arg)
{
    sk_print("main io thread started\n");
    sk_thread_env_t* thread_env = arg;
    sk_thread_env_set(thread_env);
    sk_logger_setcookie(SK_CORE_LOG_COOKIE);

    // Now, after `sk_thread_env_set`, we can use SK_THREAD_ENV_xxx macros
    sk_sched_t* sched = SK_ENV_SCHED;
    sk_sched_start(sched);
    return 0;
}

static
void* worker_io_thread(void* arg)
{
    sk_print("worker io thread started\n");
    sk_thread_env_t* thread_env = arg;
    sk_thread_env_set(thread_env);
    sk_logger_setcookie(SK_CORE_LOG_COOKIE);

    // Now, after `sk_thread_env_set`, we can use SK_THREAD_ENV_xxx macros
    sk_sched_t* sched = SK_ENV_SCHED;
    sk_sched_start(sched);
    return 0;
}

static
void _sk_init_config(sk_core_t* core)
{
    core->config = sk_config_create(core->cmd_args.config_location);
    sk_config_print(core->config);
}

static
void _sk_chdir(sk_core_t* core)
{
    char* raw_path = strdup(core->cmd_args.config_location);
    const char* config_dir = dirname(raw_path);
    int ret = chdir(config_dir);
    SK_ASSERT_MSG(!ret, "change working dir to %s failed, errno:%d\n",
                  config_dir, errno);
    free(raw_path);

    core->working_dir = getcwd(NULL, 0);
    sk_print("current working dir: %s\n", core->working_dir);
}

static
void _sk_module_init(sk_core_t* core)
{
    fhash_str_iter iter = fhash_str_iter_new(core->unique_modules);
    sk_module_t* module = NULL;

    while ((module = fhash_str_next(&iter))) {
        sk_print("module [%s] init...\n", module->name);
        sk_logger_setcookie("module.%s", module->name);
        module->init(module->md);
        sk_logger_setcookie(SK_CORE_LOG_COOKIE);
    }
}

static
void _sk_init_log(sk_core_t* core)
{
    const sk_config_t* config = core->config;
    const char* working_dir = core->working_dir;
    const char* log_name = config->log_name;
    int log_level = config->log_level;

    core->logger = sk_logger_create(working_dir, log_name, log_level);
    SK_ASSERT_MSG(core->logger, "create core logger failed\n");

    // create a thread env for the main thread, this is necessary since during
    // the phase of loading modules, user may log something, if the thread_env
    // does not exist, the logs will be dropped
    sk_thread_env_t* env = sk_thread_env_create(core, NULL, "main", 0);
    sk_thread_env_set(env);

    SK_LOG_INFO(core->logger, "skull logger initialization successfully");
}

static
void _sk_init_log_tpls(sk_core_t* core)
{
    const char* working_dir = core->working_dir;
    char log_tpl_name[SK_LOG_TPL_NAME_MAX_LEN];

    // 1. load info log template
    memset(log_tpl_name, 0, SK_LOG_TPL_NAME_MAX_LEN);
    snprintf(log_tpl_name, SK_LOG_TPL_NAME_MAX_LEN, "%s/%s",
             working_dir, SK_LOG_INFO_TPL_NAME);

    core->info_log_tpl = sk_log_tpl_create(log_tpl_name, SK_LOG_INFO_TPL);
    SK_ASSERT(core->info_log_tpl);

    // 2. load warn log template
    memset(log_tpl_name, 0, SK_LOG_TPL_NAME_MAX_LEN);
    snprintf(log_tpl_name, SK_LOG_TPL_NAME_MAX_LEN, "%s/%s",
             working_dir, SK_LOG_WARN_TPL_NAME);

    core->warn_log_tpl = sk_log_tpl_create(log_tpl_name, SK_LOG_ERROR_TPL);
    SK_ASSERT(core->warn_log_tpl);

    // 3. load error log template
    memset(log_tpl_name, 0, SK_LOG_TPL_NAME_MAX_LEN);
    snprintf(log_tpl_name, SK_LOG_TPL_NAME_MAX_LEN, "%s/%s",
             working_dir, SK_LOG_ERROR_TPL_NAME);

    core->error_log_tpl = sk_log_tpl_create(log_tpl_name, SK_LOG_ERROR_TPL);
    SK_ASSERT(core->error_log_tpl);

    // 4. load fatal log template
    memset(log_tpl_name, 0, SK_LOG_TPL_NAME_MAX_LEN);
    snprintf(log_tpl_name, SK_LOG_TPL_NAME_MAX_LEN, "%s/%s",
             working_dir, SK_LOG_FATAL_TPL_NAME);

    core->fatal_log_tpl = sk_log_tpl_create(log_tpl_name, SK_LOG_ERROR_TPL);
    SK_ASSERT(core->fatal_log_tpl);
}

static
void _sk_init_moniter(sk_core_t* core)
{
    core->mon = sk_mon_create();
    core->umon = sk_mon_create();
}

// APIs

// The skull core context initialization function, please *BE CAREFUL* for the
// execution orders, if you quite understand it, do not modify the calling ord-
// ers.
void sk_core_init(sk_core_t* core)
{
    // 1. prepare the thread env
    sk_thread_env_init();

    // 2. load config
    _sk_init_config(core);

    // 3. change working-dir to the config dir
    _sk_chdir(core);

    // 4. init logger
    _sk_init_log(core);

    // 5. init log tempaltes
    _sk_init_log_tpls(core);

    // 6. init global monitor
    _sk_init_moniter(core);

    // 7. init schedulers
    _sk_setup_engines(core);

    // 8. load working flows
    _sk_setup_workflow(core);
}

void sk_core_start(sk_core_t* core)
{
    // 1. module init
    _sk_module_init(core);

    sk_config_t* config = core->config;

    // 2. start the main io thread
    // 3. start the worker io threads
    sk_print("skull engine starting...\n");

    // 2.
    sk_engine_t* master = core->master;
    // this *main_thread_env* will be deleted when thread exit
    sk_thread_env_t* master_env = sk_thread_env_create(core, master,
                                                       "master", 0);
    int ret = sk_engine_start(master, master_env);
    if (ret) {
        sk_print("create master io thread failed, errno: %d\n", errno);
        exit(ret);
    }

    // 3.
    for (int i = 0; i < config->threads; i++) {
        sk_engine_t* worker = core->workers[i];
        // this *worker_thread_env* will be deleted when thread exit
        sk_thread_env_t* worker_env = sk_thread_env_create(core, worker,
                                                           "worker", i);
        int ret = sk_engine_start(worker, worker_env);
        if (ret) {
            sk_print("create worker io thread failed, errno: %d\n", errno);
            exit(ret);
        }
    }
    SK_LOG_INFO(core->logger, "skull engine start");

    // 4. wait them quit
    for (int i = 0; i < config->threads; i++) {
        sk_engine_wait(core->workers[i]);
    }
    sk_engine_wait(master);
}

void sk_core_stop(sk_core_t* core)
{
    sk_print("skull engine stoping...\n");
    SK_LOG_INFO(core->logger, "skull engine stop");

    if (!core) return;

    sk_engine_stop(core->master);

    for (int i = 0; i < core->config->threads; i++) {
        sk_engine_stop(core->workers[i]);
    }
}

void sk_core_destroy(sk_core_t* core)
{
    if (!core) {
        return;
    }

    sk_mon_destroy(core->mon);
    sk_mon_destroy(core->umon);

    sk_config_destroy(core->config);

    sk_engine_destroy(core->master);
    for (int i = 0; i < core->config->threads; i++) {
        sk_engine_destroy(core->workers[i]);
    }
    free(core->workers);

    sk_logger_destroy(core->logger);
    sk_log_tpl_destroy(core->info_log_tpl);
    sk_log_tpl_destroy(core->warn_log_tpl);
    sk_log_tpl_destroy(core->error_log_tpl);
    sk_log_tpl_destroy(core->fatal_log_tpl);

    flist_delete(core->workflows);

    fhash_str_delete(core->unique_modules);

    free((void*)core->working_dir);
}