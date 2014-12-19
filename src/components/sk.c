#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <dlfcn.h>

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
#include "api/sk.h"

// INTERNAL APIs

static
sk_thread_env_t* _sk_master_env_create(skull_core_t* core,
                                       skull_sched_t* master_sched,
                                       const char* name)
{
    sk_thread_env_t* thread_env = calloc(1, sizeof(*thread_env));
    thread_env->core = core;
    thread_env->sched = master_sched;

    // create a per-thread logger which use the same log name of main scheduler
    const sk_config_t* config = core->config;
    const char* working_dir = core->working_dir;
    const char* log_name = config->log_name;
    int log_level = config->log_level;
    thread_env->logger = sk_logger_create(working_dir, log_name, log_level);

    thread_env->mon = sk_mon_create();
    thread_env->monitor = sk_metrics_worker_create(name);

    return thread_env;
}

static
sk_thread_env_t* _sk_worker_env_create(skull_core_t* core,
                                          skull_sched_t* worker_sched,
                                          const char* name,
                                          int idx)
{
    sk_thread_env_t* thread_env = calloc(1, sizeof(*thread_env));
    thread_env->core = core;
    thread_env->sched = worker_sched;

    // create a per-thread logger which use the same log name of main scheduler
    const sk_config_t* config = core->config;
    const char* working_dir = core->working_dir;
    const char* log_name = config->log_name;
    int log_level = config->log_level;
    thread_env->logger = sk_logger_create(working_dir, log_name, log_level);

    thread_env->mon = sk_mon_create();
    thread_env->monitor = sk_metrics_worker_create(name);

    snprintf(thread_env->name, SK_ENV_NAME_LEN, "%s%d", name, idx);
    thread_env->idx = idx;

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
    core->worker_sched = calloc((size_t)config->threads, sizeof(skull_sched_t));

    for (int i = 0; i < config->threads; i++) {
        skull_sched_t* worker_sched = &core->worker_sched[i];

        worker_sched->evlp = sk_eventloop_create();
        worker_sched->entity_mgr = sk_entity_mgr_create(65535);
        worker_sched->sched = sk_sched_create(worker_sched->evlp,
                                              worker_sched->entity_mgr);
        SK_LOG_INFO(core->logger, "worker scheduler [%d] init successfully", i);

        sk_sched_setup_bridge(main_sched->sched, worker_sched->sched);
        SK_LOG_INFO(core->logger, "io bridge [%d] init successfully", i);
    }

    SK_LOG_INFO(core->logger, "skull schedulers init successfully");
}

static
void __sk_open_common_lib(skull_core_t* core, const char* subdir_name)
{
    DIR* subd;
    struct dirent* subdir;

    subd = opendir(subdir_name);
    if (!subd) {
        SK_LOG_ERROR(core->logger, "cannot open common lib subdir [%s]",
                     subdir_name);
        exit(1);
    }

    while ((subdir = readdir(subd)) != NULL) {
        size_t name_len = strlen(subdir->d_name);

        // a  valid so name length must be >= 4, skip the invalid files
        if (!name_len || name_len < 4) {
            continue;
        }

        // check the file suffix is .so
        if (0 == strcmp(&subdir->d_name[name_len - 3], ".so")) {
            printf("found so file: %s\n", subdir->d_name);
            char common_lib_name[256] = {0};
            snprintf(common_lib_name, 256, "%s/%s", subdir_name,
                     subdir->d_name);

            // load common library
            dlerror();
            void* handle = dlopen(common_lib_name, RTLD_NOW);
            if (!handle) {
                SK_LOG_ERROR(core->logger, "cannot load the common lib %s:%s",
                             common_lib_name, dlerror());
            }

            SK_LOG_INFO(core->logger, "load common lib %s successfully",
                        common_lib_name);
        }
    }

    closedir(subd);
}

// generally, we only need to load the c/c++ libs
static
void _skull_setup_common_lib(skull_core_t* core)
{
    char* raw_path = strdup(core->cmd_args.config_location);
    const char* config_dir = dirname(raw_path);
    char common_lib_dir[256] = {0};
    snprintf(common_lib_dir, 256, "%s/common", config_dir);
    printf("open common lib dir: %s\n", common_lib_dir);

    DIR* d;
    struct dirent* dir;
    d = opendir(common_lib_dir);
    if (!d) {
        SK_LOG_ERROR(core->logger, "cannot open common lib dir [%s]",
                     common_lib_dir);
    }

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type != DT_DIR) {
            continue;
        }

        printf("found subdir: %s\n", dir->d_name);
        char subdir_name[256] = {0};
        snprintf(subdir_name, 256, "%s/%s", common_lib_dir, dir->d_name);
        __sk_open_common_lib(core, subdir_name);
    }

    closedir(d);
    free(raw_path);
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

    SK_LOG_INFO(core->logger, "set up triggers successfully");
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

        // store this workflow to skull_core::workflows
        int ret = flist_push(core->workflows, workflow);
        SK_ASSERT(!ret);
        SK_LOG_INFO(core->logger, "setup one workflow successfully");

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
    sk_logger_setcookie(SK_CORE_LOG_COOKIE);

    // Now, after `sk_thread_env_set`, we can use SK_THREAD_ENV_xxx macros
    sk_sched_t* sched = SK_THREAD_ENV_SCHED;
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
    sk_sched_t* sched = SK_THREAD_ENV_SCHED;
    sk_sched_start(sched);
    return 0;
}

static
void _skull_init_config(skull_core_t* core)
{
    core->config = sk_config_create(core->cmd_args.config_location);
    sk_config_print(core->config);
}

static
void _skull_chdir(skull_core_t* core)
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
void _skull_module_init(skull_core_t* core)
{
    fhash_str_iter iter = fhash_str_iter_new(core->unique_modules);
    sk_module_t* module = NULL;

    while ((module = fhash_str_next(&iter))) {
        sk_print("module [%s] init...\n", module->name);
        sk_logger_setcookie("module.%s", module->name);
        module->sk_module_init();
        sk_logger_setcookie(SK_CORE_LOG_COOKIE);
    }
}

static
void _skull_init_log(skull_core_t* core)
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
    sk_thread_env_t* env = _sk_worker_env_create(core, NULL, "main", 0);
    sk_thread_env_set(env);

    SK_LOG_INFO(core->logger, "skull logger initialization successfully");
}

static
void _skull_init_log_tpls(skull_core_t* core)
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
void _skull_init_moniter(skull_core_t* core)
{
    core->mon = sk_mon_create();
    core->monitor = sk_metrics_global_create("global");
}

// APIs

// The skull core context initialization function, please *BE CAREFUL* for the
// execution orders, if you quite understand it, do not modify the calling ord-
// ers.
void skull_init(skull_core_t* core)
{
    // 1. prepare the thread env
    sk_thread_env_init();

    // 2. load config
    _skull_init_config(core);

    // 3. change working-dir to the config dir
    _skull_chdir(core);

    // 4. init logger
    _skull_init_log(core);

    // 5. init log tempaltes
    _skull_init_log_tpls(core);

    // 6. init global monitor
    _skull_init_moniter(core);

    // 7. init schedulers
    _skull_setup_schedulers(core);

    // 8. init common libraries
    _skull_setup_common_lib(core);

    // 9. load working flows
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
    sk_thread_env_t* main_thread_env = _sk_master_env_create(core,
                                                             main_sched,
                                                             "master");
    int ret = pthread_create(&main_sched->io_thread, NULL,
                             main_io_thread, main_thread_env);
    if (ret) {
        sk_print("create main io thread failed, errno: %d\n", errno);
        exit(ret);
    }

    // 3.
    for (int i = 0; i < config->threads; i++) {
        skull_sched_t* worker_sched = &core->worker_sched[i];
        // this *worker_thread_env* will be deleted when thread exit
        sk_thread_env_t* worker_thread_env = _sk_worker_env_create(core,
                                                               worker_sched,
                                                               "worker",
                                                               i);
        ret = pthread_create(&worker_sched->io_thread, NULL,
                             worker_io_thread, worker_thread_env);
        if (ret) {
            sk_print("create worker io thread failed, errno: %d\n", errno);
            exit(ret);
        }
    }
    SK_LOG_INFO(core->logger, "skull engine start");

    // 4. wait them quit
    for (int i = 0; i < config->threads; i++) {
        pthread_join(core->worker_sched[i].io_thread, NULL);
    }
    pthread_join(main_sched->io_thread, NULL);
}

void skull_stop(skull_core_t* core)
{
    sk_print("skull engine stoping...\n");
    SK_LOG_INFO(core->logger, "skull engine stop");
    for (int i = 0; i < core->config->threads; i++) {
        sk_sched_destroy(core->worker_sched[i].sched);
    }

    free(core->worker_sched);
    sk_sched_destroy(core->main_sched.sched);
    sk_config_destroy(core->config);
    flist_delete(core->workflows);
    fhash_str_delete(core->unique_modules);
    sk_logger_destroy(core->logger);

    // destroy the monitor
    sk_metrics_global_destroy(core->monitor);
    sk_mon_destroy(core->mon);

    // destroy log templates
    sk_log_tpl_destroy(core->info_log_tpl);
    sk_log_tpl_destroy(core->warn_log_tpl);
    sk_log_tpl_destroy(core->error_log_tpl);
    sk_log_tpl_destroy(core->fatal_log_tpl);
}

