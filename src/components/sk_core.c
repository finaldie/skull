#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <libgen.h>

#include "api/sk_types.h"
#include "api/sk_utils.h"
#include "api/sk_eventloop.h"
#include "api/sk_pto.h"
#include "api/sk_config.h"
#include "api/sk_module.h"
#include "api/sk_admin.h"
#include "api/sk_service.h"
#include "api/sk_workflow.h"
#include "api/sk_trigger.h"
#include "api/sk_loader.h"
#include "api/sk_const.h"
#include "api/sk_env.h"
#include "api/sk_log.h"
#include "api/sk_log_tpl.h"
#include "api/sk_core.h"

// INTERNAL APIs

static
void _sk_init_admin(sk_core_t* core)
{
    sk_print("Init admin module\n");
    SK_LOG_INFO(core->logger, "Init admin module");

    core->admin_wf_cfg = sk_admin_workflowcfg_create(7759);
    core->admin_wf     = sk_workflow_create(core->admin_wf_cfg);

    sk_module_t* admin_module = sk_admin_module();
    sk_workflow_add_module(core->admin_wf, admin_module);

    sk_trigger_t* trigger = sk_trigger_create(core->master, core->admin_wf,
                                              core->admin_wf_cfg);
    int ret = flist_push(core->triggers, trigger);
    SK_ASSERT(!ret);
}

static
void _sk_admin_destroy(sk_core_t* core)
{
    sk_print("Destroying admin module\n");
    SK_LOG_INFO(core->logger, "Destroying admin module");

    sk_workflow_destroy(core->admin_wf);
    sk_admin_workflowcfg_destroy(core->admin_wf_cfg);
}

static
void _sk_setup_engines(sk_core_t* core)
{
    sk_config_t* config = core->config;

    // 1. Create master engine
    core->master = sk_engine_create(SK_ENGINE_MASTER, 0);
    SK_LOG_INFO(core->logger, "master engine init successfully");

    // 2. Create worker engines
    core->workers = calloc((size_t)config->threads, sizeof(sk_engine_t*));

    for (int i = 0; i < config->threads; i++) {
        core->workers[i] = sk_engine_create(SK_ENGINE_WORKER, 0);
        SK_LOG_INFO(core->logger, "worker engine [%d] init successfully", i);

        // create both-way links
        //  - create a link from master to worker
        sk_engine_link(core->master, core->workers[i]);

        //  - create a link from worker to master
        sk_engine_link(core->workers[i], core->master);

        SK_LOG_INFO(core->logger, "io bridge [%d] init successfully", i);
    }

    // 3. Create background io engine
    core->bio     = calloc((size_t)config->bio_cnt, sizeof(sk_engine_t*));
    core->bio_map = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    flist_iter iter = flist_new_iter(config->bio);
    const char* bio_name = NULL;
    int bio_idx = 0;

    while ((bio_name = flist_each(&iter))) {
        sk_engine_t* bio = sk_engine_create(SK_ENGINE_BIO, SK_SCHED_NON_RR_ROUTABLE);
        sk_engine_link(bio, core->master);
        sk_engine_link(core->master, bio);

        core->bio[bio_idx++] = bio;
        fhash_str_set(core->bio_map, bio_name, bio);

        SK_LOG_INFO(core->logger, "bio engine [%s] init successfully", bio_name);
    }

    SK_LOG_INFO(core->logger, "skull engines init successfully");
}

static
void _sk_setup_workflows(sk_core_t* core)
{
    sk_config_t* config = core->config;
    core->workflows      = flist_create();
    core->triggers       = flist_create();
    core->unique_modules = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    flist_iter iter = flist_new_iter(config->workflows);
    sk_workflow_cfg_t* workflow_cfg = NULL;

    while ((workflow_cfg = flist_each(&iter))) {
        sk_print("setup workflow, detail info:\n");
        SK_LOG_INFO(core->logger, "setup one workflow...");

        // set up the type and concurrent
        sk_workflow_t* workflow = sk_workflow_create(workflow_cfg);

        // set up modules
        flist_iter name_iter = flist_new_iter(workflow_cfg->modules);
        char* module_name = NULL;
        while ((module_name = flist_each(&name_iter))) {
            sk_print("loading module: %s\n", module_name);

            // 1. check whether has loaded
            // 2. Load the module
            // 3. Add the module into workflow
            // 4. Store it to the unique module list to avoid load one module
            //    twice

            // 1.
            if (fhash_str_get(core->unique_modules, module_name)) {
                sk_print("we have already loaded this module(%s), skip it\n",
                         module_name);
                continue;
            }

            // 2.
            sk_module_t* module = sk_module_load(module_name, NULL);
            if (module) {
                sk_print("load module [%s] successful\n", module_name);
                SK_LOG_INFO(core->logger, "load module [%s] successful",
                          module_name);
            } else {
                sk_print("load module [%s] failed\n", module_name);
                SK_LOG_FATAL(core->logger, "load module [%s] failed",
                             module_name);
                exit(1);
            }

            // 3.
            int ret = sk_workflow_add_module(workflow, module);
            SK_ASSERT_MSG(!ret, "add module {%s} to workflow failed\n",
                          module_name);

            // 4.
            fhash_str_set(core->unique_modules, module_name, module);
        }

        // store this workflow to sk_core::workflows
        int ret = flist_push(core->workflows, workflow);
        SK_ASSERT(!ret);

        // setup triggers
        sk_trigger_t* trigger = sk_trigger_create(core->master, workflow,
                                                  workflow_cfg);
        ret = flist_push(core->triggers, trigger);
        SK_ASSERT(!ret);

        SK_LOG_INFO(core->logger, "setup one workflow successfully");
    }
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
        SK_LOG_INFO(core->logger, "module [%s] init...", module->name);

        sk_logger_setcookie("module.%s", module->name);
        module->init(module->md);
        sk_logger_setcookie(SK_CORE_LOG_COOKIE);
    }

    fhash_str_iter_release(&iter);
}

static
void _sk_module_destroy(sk_core_t* core)
{
    fhash_str_iter iter = fhash_str_iter_new(core->unique_modules);
    sk_module_t* module = NULL;

    while ((module = fhash_str_next(&iter))) {
        // 1. release module user layer data
        sk_logger_setcookie("module.%s", module->name);
        module->release(module->md);
        sk_logger_setcookie(SK_CORE_LOG_COOKIE);

        // 2. release module core layer data
        sk_module_unload(module);
    }

    fhash_str_iter_release(&iter);
    fhash_str_delete(core->unique_modules);
}

static
void _sk_setup_services(sk_core_t* core)
{
    sk_config_t* config = core->config;
    core->services = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    fhash_str_iter srv_cfg_iter = fhash_str_iter_new(config->services);
    sk_service_cfg_t* srv_cfg_item = NULL;

    while ((srv_cfg_item = fhash_str_next(&srv_cfg_iter))) {
        const char* service_name = srv_cfg_iter.key;
        SK_LOG_INFO(core->logger, "setup service %s", service_name);

        // create and load a service
        sk_service_t* service = sk_service_create(service_name, srv_cfg_item);
        int ret = sk_service_load(service, NULL);
        if (ret) {
            SK_LOG_FATAL(core->logger, "setup service %s failed", service_name);
            exit(1);
        }

        // store service into core->service hash table
        fhash_str_set(core->services, service_name, service);

        SK_LOG_INFO(core->logger, "setup service %s successful", service_name);
    }

    fhash_str_iter_release(&srv_cfg_iter);
}

static
void _sk_service_init(sk_core_t* core)
{
    fhash_str_iter srv_iter = fhash_str_iter_new(core->services);
    sk_service_t* service = NULL;

    while ((service = fhash_str_next(&srv_iter))) {
        const char* service_name = sk_service_name(service);
        sk_print("init service %s\n", service_name);
        SK_LOG_INFO(core->logger, "init service %s", service_name);

        sk_logger_setcookie("service.%s", service_name);
        sk_service_start(service);
        sk_logger_setcookie(SK_CORE_LOG_COOKIE);
    }

    fhash_str_iter_release(&srv_iter);
}

static
void _sk_service_destroy(sk_core_t* core)
{
    fhash_str_iter srv_iter = fhash_str_iter_new(core->services);
    sk_service_t* service = NULL;

    while ((service = fhash_str_next(&srv_iter))) {
        const char* service_name = sk_service_name(service);
        SK_LOG_INFO(core->logger, "destroy service %s", service_name);

        // 1. release service user layer data
        sk_logger_setcookie("service.%s", service_name);
        sk_service_stop(service);
        sk_logger_setcookie(SK_CORE_LOG_COOKIE);

        // 2. release service core layer data
        sk_service_unload(service);
        sk_service_destroy(service);
        SK_LOG_INFO(core->logger, "destroy service %s complete", service_name);
    }

    fhash_str_iter_release(&srv_iter);
    fhash_str_delete(core->services);
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

    // create a thread env for the master thread, this is necessary since during
    // the phase of loading modules, user may log something, if the thread_env
    // does not exist, the logs will be dropped
    sk_thread_env_t* env = sk_thread_env_create(core, NULL, "master");
    sk_thread_env_set(env);

    SK_LOG_TRACE(core->logger, "skull logger initialization successfully");
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

static
void _sk_engines_destroy(sk_core_t* core)
{
    // 1. destroy workers
    for (int i = 0; i < core->config->threads; i++) {
        sk_engine_destroy(core->workers[i]);
    }
    free(core->workers);

    // 2. destroy bio(s)
    for (int i = 0; i < core->config->bio_cnt; i++) {
        sk_engine_destroy(core->bio[i]);
    }
    fhash_str_delete(core->bio_map);
    free(core->bio);

    // 3. destroy master
    sk_engine_destroy(core->master);
}

// APIs

// The skull core context initialization function, please *BE CAREFUL* for the
// execution orders, if you quite understand it, do not modify the calling ord-
// ers.
void sk_core_init(sk_core_t* core)
{
    core->status = SK_CORE_INIT;

    // 1. prepare the thread env
    sk_thread_env_init();

    // 2. load config
    _sk_init_config(core);

    // 3. change working-dir to the config dir
    _sk_chdir(core);

    // 4. init logger
    _sk_init_log(core);

    SK_LOG_INFO(core->logger,
             "================= skull engine initializing =================");
    sk_print("================= skull engine initializing =================\n");

    // 5. init log tempaltes
    _sk_init_log_tpls(core);

    // 6. init global monitor
    _sk_init_moniter(core);

    // 7. init engines
    _sk_setup_engines(core);

    // 8. load workflows and related triggers
    _sk_setup_workflows(core);

    // 9. init admin
    _sk_init_admin(core);

    // 10. load services
    _sk_setup_services(core);
}

void sk_core_start(sk_core_t* core)
{
    SK_LOG_INFO(core->logger,
             "================== skull engine starting ==================");
    sk_print("================== skull engine starting ==================\n");
    core->status = SK_CORE_STARTING;

    // 1. Complete the master_env
    //   notes: This *master_env* will be deleted when thread exit
    sk_engine_t*     master     = core->master;
    sk_thread_env_t* master_env = sk_thread_env();
    master_env->engine = master;

    // 2. module init
    _sk_module_init(core);

    // 3. service init
    _sk_service_init(core);

    // 4. start worker engines
    SK_LOG_INFO(core->logger, "starting worker engines...");
    sk_print("starting workers...\n");

    sk_config_t* config = core->config;

    for (int i = 0; i < config->threads; i++) {
        sk_engine_t* worker = core->workers[i];
        // this *worker_thread_env* will be deleted when thread exit
        sk_thread_env_t* worker_env = sk_thread_env_create(core, worker,
                                                           "worker-%d", i);
        int ret = sk_engine_start(worker, worker_env, 1);
        if (ret) {
            sk_print("Start worker io engine failed, errno: %d\n", errno);
            exit(ret);
        }
    }

    // 5. start bio engines
    fhash_str_iter bio_iter = fhash_str_iter_new(core->bio_map);
    sk_engine_t* bio = NULL;

    while ((bio = fhash_str_next(&bio_iter))) {
        const char* bio_name = bio_iter.key;
        sk_thread_env_t* bio_env =
            sk_thread_env_create(core, bio, "bio-%s", bio_name);

        int ret = sk_engine_start(bio, bio_env, 1);
        if (ret) {
            sk_print("Start bio engine failed, errno: %d\n", errno);
            exit(ret);
        }

        SK_LOG_INFO(core->logger, "Start bio engine [%s] successfully", bio_name);
    }
    fhash_str_iter_release(&bio_iter);

    // 6. start triggers
    SK_LOG_INFO(core->logger, "starting triggers...");
    sk_print("starting triggers...\n");

    flist_iter iter = flist_new_iter(core->triggers);
    sk_trigger_t* trigger = NULL;
    while ((trigger = flist_each(&iter))) {
        sk_trigger_run(trigger);
    }

    // 7. update core status
    core->status = SK_CORE_SERVING;

    // 8. start master engine
    SK_LOG_INFO(core->logger,
             "================== skull engine is ready ====================");
    sk_print("================== skull engine is ready ====================\n");

    int ret = sk_engine_start(master, master_env, 0);
    if (ret) {
        sk_print("Start master io engine failed, errno: %d\n", errno);
        exit(ret);
    }

    // 9. wait worker(s) and bio(s) quit
    // 9.1 wait for worker(s)
    for (int i = 0; i < config->threads; i++) {
        sk_engine_wait(core->workers[i]);
    }

    // 9.2 wait for bio(s)
    for (int i = 0; i < core->config->bio_cnt; i++) {
        sk_engine_wait(core->bio[i]);
    }
}

void sk_core_stop(sk_core_t* core)
{
    if (!core) return;

    SK_LOG_INFO(core->logger,
             "=================== skull engine stopping ===================");
    sk_print("=================== skull engine stopping ===================\n");
    core->status = SK_CORE_STOPPING;

    // 1. stop master
    sk_engine_stop(core->master);

    // 2. stop workers
    for (int i = 0; i < core->config->threads; i++) {
        sk_engine_stop(core->workers[i]);
    }

    // 3. stop bio(s)
    for (int i = 0; i < core->config->bio_cnt; i++) {
        sk_engine_stop(core->bio[i]);
    }
}

void sk_core_destroy(sk_core_t* core)
{
    if (!core) {
        return;
    }

    SK_LOG_INFO(core->logger,
             "================== skull engine destroying ==================");
    sk_print("================== skull engine destroying ==================\n");
    core->status = SK_CORE_DESTROYING;

    // 1. destroy triggers
    sk_trigger_t* trigger = NULL;
    while ((trigger = flist_pop(core->triggers))) {
        sk_trigger_destroy(trigger);
    }
    flist_delete(core->triggers);

    // 2. destroy engines
    _sk_engines_destroy(core);

    // 3. destroy modules
    _sk_module_destroy(core);

    // 4. destroy services
    _sk_service_destroy(core);

    // 5. destroy moniters
    sk_mon_destroy(core->mon);
    sk_mon_destroy(core->umon);

    // 6. destroy config
    sk_config_destroy(core->config);

    // 7. destroy workflows
    sk_workflow_t* workflow = NULL;
    while ((workflow = flist_pop(core->workflows))) {
        sk_workflow_destroy(workflow);
    }
    flist_delete(core->workflows);

    // 8. destroy admin
    _sk_admin_destroy(core);

    // 9. destroy working dir string
    free((void*)core->working_dir);

    // 10. destroy loggers
    sk_print("=================== skull engine stopped ====================\n");
    SK_LOG_INFO(core->logger,
             "=================== skull engine stopped ====================");

    sk_logger_destroy(core->logger);
    sk_log_tpl_destroy(core->info_log_tpl);
    sk_log_tpl_destroy(core->warn_log_tpl);
    sk_log_tpl_destroy(core->error_log_tpl);
    sk_log_tpl_destroy(core->fatal_log_tpl);
}

// Utils APIs
sk_service_t* sk_core_service(sk_core_t* core, const char* service_name)
{
    SK_ASSERT(core);
    SK_ASSERT(service_name);

    return fhash_str_get(core->services, service_name);
}

sk_core_status_t sk_core_status(sk_core_t* core)
{
    return core->status;
}

sk_engine_t*     sk_core_bio(sk_core_t* core, const char* name)
{
    return fhash_str_get(core->bio_map, name);
}
