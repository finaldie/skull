#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

#include "api/sk_admin.h"
#include "api/sk_config.h"
#include "api/sk_const.h"
#include "api/sk_core.h"
#include "api/sk_driver.h"
#include "api/sk_env.h"
#include "api/sk_eventloop.h"
#include "api/sk_loader.h"
#include "api/sk_log.h"
#include "api/sk_log_helper.h"
#include "api/sk_malloc.h"
#include "api/sk_module.h"
#include "api/sk_pto.h"
#include "api/sk_service.h"
#include "api/sk_time.h"
#include "api/sk_types.h"
#include "api/sk_utils.h"
#include "api/sk_workflow.h"

// INTERNAL APIs

static
int _sk_snprintf_logpath(char* path, size_t sz, const char* workdir,
                         const char* logname, bool std_fwd) {
    if (std_fwd) {
        return snprintf(path, SK_LOG_MAX_PATH_LEN, "%s", SK_LOG_STDOUT_FILE);
    }

    if (logname[0] == '/') {
        return snprintf(path, SK_LOG_MAX_PATH_LEN, "%s", logname);
    }

    size_t workdir_len = strlen(workdir);
    size_t logname_len = strlen(logname);

    // Notes: we add more 5 bytes space for the string of fullname "/log/"
    size_t full_name_sz = workdir_len + logname_len + 5 + 1;
    SK_ASSERT_MSG(full_name_sz < SK_LOG_MAX_PATH_LEN,
                  "Full log path is too long (>= %d), workdir: %s, name: %s\n",
                  SK_LOG_MAX_PATH_LEN, workdir, logname);

    // Construct the full log name and then create async logger
    // NOTES: the log file will be put at log/xxx
    return snprintf(path, SK_LOG_MAX_PATH_LEN, "%s/log/%s", workdir, logname);
}

static
void _sk_init_mem_log(sk_core_t* core) {
    const sk_config_t*   config   =  core->config;
    const sk_cmd_args_t* cmd_args = &core->cmd_args;

    _sk_snprintf_logpath(core->diag_path, SK_LOG_MAX_PATH_LEN,
                         core->working_dir, config->diag_name,
                         cmd_args->log_stdout_fwd);

    core->logger_diag = sk_logger_create(core->diag_path,
                         config->log_level,
                         cmd_args->log_rolling_disabled,
                         cmd_args->log_stdout_fwd);
    SK_ASSERT_MSG(core->logger_diag, "create diagnosis logger failed\n");

    sk_mem_init_log(core->logger_diag);
}

static
void _sk_mem_destroy(sk_core_t* core) {
    sk_mem_destroy();
}

static
void _sk_init_admin(sk_core_t* core) {
    sk_print("Init admin module\n");
    SK_LOG_INFO(core->logger, "Init admin module");

    core->drivers     = core->drivers ? core->drivers : flist_create();
    core->admin_wf_cfg = sk_admin_workflowcfg_create(core->config->command_bind,
                                                     core->config->command_port);
    core->admin_wf     = sk_workflow_create(core->admin_wf_cfg);

    sk_module_t* admin_module = sk_admin_module();
    sk_workflow_add_module(core->admin_wf, admin_module);

    sk_driver_t* driver = sk_driver_create(core->master, core->admin_wf);
    int ret = flist_push(core->drivers, driver);
    SK_ASSERT(!ret);

    sk_mem_stat_module_register(admin_module->cfg->name);
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
    core->master = sk_engine_create(SK_ENGINE_MASTER, core->max_fds, 0);
    sk_mem_dump("ENGINE-INIT-MASTER-DONE");

    // 1.1 Now, fix the master env 'engine' link
    core->env->engine = core->master;
    SK_LOG_INFO(core->logger, "master engine init successfully");

    // 2. Create worker engines
    core->workers = calloc((size_t)config->threads, sizeof(sk_engine_t*));

    for (int i = 0; i < config->threads; i++) {
        core->workers[i] = sk_engine_create(SK_ENGINE_WORKER, core->max_fds, 0);
        SK_LOG_INFO(core->logger, "worker engine [%d] init successfully", i);

        // create both-way links
        //  - create a link from master to worker
        sk_engine_link(core->master, core->workers[i]);

        //  - create a link from worker to master
        sk_engine_link(core->workers[i], core->master);

        SK_LOG_INFO(core->logger, "io bridge [%d] init successfully", i);
        sk_mem_dump("ENGINE-INIT-WORKER-DONE-%d", i);
    }

    // 3. Create background io engine
    core->bio = calloc((size_t)config->bio_cnt, sizeof(sk_engine_t*));

    for (int i = 0; i < core->config->bio_cnt; i++) {
        sk_engine_t* bio = sk_engine_create(SK_ENGINE_BIO, core->max_fds,
                                            SK_SCHED_NON_RR_ROUTABLE);
        sk_engine_link(bio, core->master);
        sk_engine_link(core->master, bio);

        core->bio[i] = bio;
        SK_LOG_INFO(core->logger, "bio engine [%d] init successfully", i + 1);
        sk_mem_dump("ENGINE-INIT-BIO-DONE-%d", i);
    }

    SK_LOG_INFO(core->logger, "skull engines init successfully");
}

static
void _sk_setup_workflows(sk_core_t* core)
{
    sk_config_t* config = core->config;
    core->workflows      = flist_create();
    core->drivers       = core->drivers ? core->drivers : flist_create();
    core->unique_modules = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    flist_iter iter = flist_new_iter(config->workflows);
    sk_workflow_cfg_t* workflow_cfg = NULL;

    while ((workflow_cfg = flist_each(&iter))) {
        sk_print("setup workflow, detail info:\n");
        SK_LOG_INFO(core->logger, "setup one workflow...");

        // set up the type and concurrency
        sk_workflow_t* workflow = sk_workflow_create(workflow_cfg);

        // set up modules
        flist_iter name_iter = flist_new_iter(workflow_cfg->modules);
        sk_module_cfg_t* module_cfg = NULL;
        while ((module_cfg = flist_each(&name_iter))) {
            sk_print("loading module: %s\n", module_cfg->name);

            // 1. check whether has loaded
            // 2. Load the module
            // 3. Add the module into workflow
            // 4. Store it to the unique module list to avoid load one module
            //    twice

            // 1.
            if (fhash_str_get(core->unique_modules, module_cfg->name)) {
                sk_print("we have already loaded this module(%s), skip it\n",
                         module_cfg->name);
                continue;
            }

            // 2.
            sk_module_t* module = sk_module_load(module_cfg, NULL);
            if (module) {
                sk_print("load module [%s] successful\n", module_cfg->name);
                SK_LOG_INFO(core->logger, "load module [%s] successful",
                          module_cfg->name);
            } else {
                sk_print("load module [%s] failed\n", module_cfg->name);
                SK_LOG_FATAL(core->logger, "load module [%s] failed",
                             module_cfg->name);
                exit(1);
            }

            // 3.
            int ret = sk_workflow_add_module(workflow, module);
            SK_ASSERT_MSG(!ret, "add module {%s} to workflow failed\n",
                          module_cfg->name);

            // 4.
            fhash_str_set(core->unique_modules, module_cfg->name, module);
        }

        // store this workflow to sk_core::workflows
        int ret = flist_push(core->workflows, workflow);
        SK_ASSERT(!ret);

        // setup driver
        sk_driver_t* driver = sk_driver_create(core->master, workflow);
        ret = flist_push(core->drivers, driver);
        SK_ASSERT(!ret);

        SK_LOG_INFO(core->logger, "setup one workflow successfully");
    }
}

static
void _sk_init_env(sk_core_t* core) {
    sk_time_init();
    sk_thread_env_init();

    // Create a thread env for the main thread, this is necessary since during
    //  the phase of loading modules, user may log something, if the thread_env
    //  does not exist, the logs will be dropped
    //
    // Notes: Once master engine started, this env will be used by
    //        master->env directly
    core->env = sk_thread_env_create(core, NULL, "master");
    sk_thread_env_set(core->env);

    sk_mem_init();
}

static
void _sk_env_destroy(sk_core_t* core) {
    _sk_mem_destroy(core);
    free(core->env);
}

static
void _sk_init_config(sk_core_t* core) {
    core->config = sk_config_create(core->cmd_args.config_location);

#ifdef SK_DEBUG
    sk_config_print(core->config);
#endif
}

static
void _sk_chdir(sk_core_t* core) {
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
void _sk_module_init(sk_core_t* core) {
    fhash_str_iter iter = fhash_str_iter_new(core->unique_modules);
    sk_module_t* module = NULL;

    while ((module = fhash_str_next(&iter))) {
        const char* module_name = module->cfg->name;
        sk_print("module [%s] init...\n", module_name);
        SK_LOG_INFO(core->logger, "module [%s] init...", module_name);

        sk_mem_stat_module_register(module_name);

        SK_LOG_SETCOOKIE("module.%s", module_name);
        if (module->init(module->md)) {
            SK_LOG_FATAL(core->logger, "Initialize module %s failed, ABORT!",
                         module_name);
            exit(1);
        }
        SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);

        sk_mem_dump("ENGINE-INIT-MODULE-DONE-%s", module_name);
    }

    fhash_str_iter_release(&iter);
}

static
void _sk_module_destroy(sk_core_t* core) {
    fhash_str_iter iter = fhash_str_iter_new(core->unique_modules);
    sk_module_t* module = NULL;

    while ((module = fhash_str_next(&iter))) {
        SK_LOG_INFO(core->logger, "Module %s is destroying", module->cfg->name);

        // 1. release module user layer data
        SK_LOG_SETCOOKIE("module.%s", module->cfg->name);
        module->release(module->md);
        SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);

        // 2. release module core layer data
        sk_module_unload(module);
    }

    fhash_str_iter_release(&iter);
    fhash_str_delete(core->unique_modules);
}

static
void _sk_setup_services(sk_core_t* core) {
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
void _sk_service_init(sk_core_t* core) {
    fhash_str_iter srv_iter = fhash_str_iter_new(core->services);
    sk_service_t* service = NULL;

    while ((service = fhash_str_next(&srv_iter))) {
        const char* service_name = sk_service_name(service);
        sk_print("init service %s\n", service_name);
        SK_LOG_INFO(core->logger, "init service %s", service_name);

        sk_mem_stat_service_register(service_name);

        SK_LOG_SETCOOKIE("service.%s", service_name);
        if (sk_service_start(service)) {
            SK_LOG_FATAL(core->logger, "Initialize service %s failed",
                         service_name);
            exit(1);
        }
        SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);
        sk_mem_dump("ENGINE-INIT-SERVICE-DONE-%s", service_name);
    }

    fhash_str_iter_release(&srv_iter);
}

static
void _sk_service_destroy(sk_core_t* core) {
    fhash_str_iter srv_iter = fhash_str_iter_new(core->services);
    sk_service_t* service = NULL;

    while ((service = fhash_str_next(&srv_iter))) {
        const char* service_name = sk_service_name(service);
        SK_LOG_INFO(core->logger, "destroy service %s", service_name);

        // 1. release service user layer data
        SK_LOG_SETCOOKIE("service.%s", service_name);
        sk_service_stop(service);
        SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);

        // 2. release service core layer data
        sk_service_unload(service);
        sk_service_destroy(service);
        SK_LOG_INFO(core->logger, "destroy service %s complete", service_name);
    }

    fhash_str_iter_release(&srv_iter);
    fhash_str_delete(core->services);
}

static
void _sk_init_log(sk_core_t* core) {
    const sk_config_t*   config   =  core->config;
    const sk_cmd_args_t* cmd_args = &core->cmd_args;

    // memory statistics logger initialization
    _sk_init_mem_log(core);

    // core logger initialization
    _sk_snprintf_logpath(core->log_path, SK_LOG_MAX_PATH_LEN, core->working_dir,
                         config->log_name, cmd_args->log_stdout_fwd);

    core->logger = sk_logger_create(core->log_path,
                         config->log_level,
                         cmd_args->log_rolling_disabled,
                         cmd_args->log_stdout_fwd);
    SK_ASSERT_MSG(core->logger, "create core logger failed\n");

    // set the core logging cookie
    SK_LOG_SETCOOKIE(SK_CORE_LOG_COOKIE, NULL);

    SK_LOG_TRACE(core->logger, "skull logger initialization successfully");
}

static
void _sk_init_moniter(sk_core_t* core) {
    core->mon  = sk_mon_create();
    core->umon = sk_mon_create();
}

static
void _sk_engines_destroy(sk_core_t* core) {
    // 1. destroy workers
    for (int i = 0; i < core->config->threads; i++) {
        SK_LOG_INFO(core->logger, "Engine worker-%d is destroying", i);
        sk_engine_destroy(core->workers[i]);
    }
    free(core->workers);

    // 2. destroy bio(s)
    for (int i = 0; i < core->config->bio_cnt; i++) {
        SK_LOG_INFO(core->logger, "Engine bio-%d is destroying", i);
        sk_engine_destroy(core->bio[i]);
    }
    free(core->bio);

    // 3. destroy master
    sk_engine_destroy(core->master);
}

static
void _sk_init_user_loaders(sk_core_t* core) {
    core->apis = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    sk_config_t* cfg = core->config;
    fhash* user_langs = cfg->langs;
    const char* lang = NULL;

    fhash_str_iter iter = fhash_str_iter_new(user_langs);
    while ((fhash_str_next(&iter))) {
        lang = iter.key;

        // 1. generate user library name
        char mlibname[SK_MODULE_NAME_MAX_LEN];
        memset(mlibname, 0, SK_MODULE_NAME_MAX_LEN);
        snprintf(mlibname, SK_MODULE_NAME_MAX_LEN, SK_USER_LIBNAME_FORMAT, lang);
        sk_print("Loading user api layer: %s\n", mlibname);
        SK_LOG_INFO(core->logger, "Loading user api layer: %s", mlibname);

        // 2. load module and service loader
        int ret = sk_userlib_load(mlibname);
        if (ret) {
            SK_LOG_WARN(core->logger, "Load user lib %s failed", mlibname);
            fhash_str_set(core->apis, lang, "Not loaded");
        } else {
            fhash_str_set(core->apis, lang, "Loaded");
        }
    }
    fhash_str_iter_release(&iter);
}

static
void _sk_destroy_user_loaders(sk_core_t* core) {
    sk_userlib_unload();
    fhash_str_delete(core->apis);
}

static
void _sk_init_sys(sk_core_t* core) {
    // 1. Set the max open file if needed
    // 1.1 get the current open file limitation (soft)
    struct rlimit limit;
    int ret = getrlimit(RLIMIT_NOFILE, &limit);
    if (ret) {
        fprintf(stderr, "Error: getrlimit failed\n");
        SK_LOG_FATAL(core->logger, "getrlimit failed");
        exit(1);
    }

    // 1.2 Set the new limitation if needed
    int soft_limit = (int)limit.rlim_cur;
    int conf_max_fds = core->config->max_fds;
    core->max_fds = SK_MIN(conf_max_fds, soft_limit);

    SK_LOG_INFO(core->logger, "Open files limitation (soft): %d,"
                " config max_fds: %d, set max_fds to: %d",
                soft_limit, core->config->max_fds, core->max_fds);

    struct rlimit new_limit;
    new_limit.rlim_cur = (rlim_t)core->max_fds;
    new_limit.rlim_max = (rlim_t)core->max_fds;

    if (setrlimit(RLIMIT_NOFILE, &new_limit)) {
        SK_LOG_WARN(core->logger,
                    "set max open file limitation failed: %s, fallback to "
                    "soft-limit value: %d",
                    strerror(errno), soft_limit);

        core->max_fds = soft_limit;
    }
}

static
void _sk_init_coreinfo(sk_core_t* core) {
    sk_util_setup_coreinfo(core);
}

// APIs

// The skull core context initialization function, please *BE CAREFUL* for the
// execution orders, DO NOT modify the order before fully understand it.
void sk_core_init(sk_core_t* core) {
    core->status = SK_CORE_INIT;

    // 1. prepare the thread env
    _sk_init_env(core);

    // 2. load config
    _sk_init_config(core);

    // 3. change working-dir to the config dir
    _sk_chdir(core);

    // 4. init logger
    _sk_init_log(core);

    sk_mem_dump("ENGINE-INIT");

    SK_LOG_INFO(core->logger,
             "================= skull engine initializing =================");
    sk_print("================= skull engine initializing =================\n");

    // 5. init system level parameters
    _sk_init_sys(core);
    sk_mem_dump("ENGINE-INIT-SYS-DONE");

    // 6. loader user loaders
    _sk_init_user_loaders(core);
    sk_mem_dump("ENGINE-INIT-LOADER-DONE");

    // 7. init global monitor
    _sk_init_moniter(core);
    sk_mem_dump("ENGINE-INIT-MONITORING-DONE");

    // 8. init engines
    _sk_setup_engines(core);
    sk_mem_dump("ENGINE-INIT-ENGINES-DONE");

    // 9. init admin
    _sk_init_admin(core);
    sk_mem_dump("ENGINE-INIT-ADMIN-DONE");

    // 10. load services
    _sk_setup_services(core);
    sk_mem_dump("ENGINE-INIT-SERVICES-DONE");

    // 11. load workflows and related drivers
    _sk_setup_workflows(core);
    sk_mem_dump("ENGINE-INIT-WORKFLOWS-DONE");

    // 12. fill up static information
    _sk_init_coreinfo(core);

    sk_mem_dump("ENGINE-INIT-DONE");
}

void sk_core_start(sk_core_t* core) {
    core->status = SK_CORE_STARTING;

    sk_mem_dump("ENGINE-START");
    SK_LOG_INFO(core->logger,
             "================== skull engine starting ==================");
    sk_print("================== skull engine starting ==================\n");

    // 1. module init
    _sk_module_init(core);

    // 2. service init
    _sk_service_init(core);

    // 3. start worker engines
    SK_LOG_INFO(core->logger, "starting worker engines...");
    sk_print("starting workers...\n");

    sk_config_t* config = core->config;

    for (int i = 0; i < config->threads; i++) {
        sk_engine_t* worker = core->workers[i];
        sk_thread_env_t* worker_env =
            sk_thread_env_create(core, worker, "worker-%d", i);

        int ret = sk_engine_start(worker, worker_env, 1);
        if (ret) {
            sk_print("Start worker io engine failed, errno: %d\n", errno);
            exit(ret);
        }

        SK_LOG_INFO(core->logger, "Start worker engine [%d] successfully", i);
    }

    // 4. start bio engines
    for (int i = 0; i < core->config->bio_cnt; i++) {
        sk_engine_t* bio = core->bio[i];

        sk_thread_env_t* bio_env =
            sk_thread_env_create(core, bio, "bio-%d", i + 1);

        int ret = sk_engine_start(bio, bio_env, 1);
        if (ret) {
            sk_print("Start bio engine failed, errno: %d\n", errno);
            exit(ret);
        }

        SK_LOG_INFO(core->logger, "Start bio engine [%d] successfully", i + 1);
    }

    // 5. Active drivers
    SK_LOG_INFO(core->logger, "starting drivers...");
    sk_print("starting drivers...\n");

    flist_iter iter = flist_new_iter(core->drivers);
    sk_driver_t* driver = NULL;
    while ((driver = flist_each(&iter))) {
        sk_driver_run(driver);
    }

    // 6. update core status and related information
    core->status    = SK_CORE_RUNNING;
    core->starttime = time(NULL);

    // 7. start master engine
    SK_LOG_INFO(core->logger,
             "================== skull engine is ready ====================");
    sk_print("================== skull engine is ready ====================\n");
    sk_mem_dump("ENGINE-START-DONE");

    int ret = sk_engine_start(core->master, core->env, 0);
    if (ret) {
        sk_print("Start master io engine failed, errno: %d\n", errno);
        exit(ret);
    }

    // 8. wait worker(s) and bio(s) quit
    // 8.1 wait for worker(s)
    for (int i = 0; i < config->threads; i++) {
        sk_engine_wait(core->workers[i]);
    }

    // 8.2 wait for bio(s)
    for (int i = 0; i < core->config->bio_cnt; i++) {
        sk_engine_wait(core->bio[i]);
    }
}

void sk_core_stop(sk_core_t* core) {
    core->status = SK_CORE_STOPPING;

    // Stop tracing if enabled
    sk_mem_trace(0);
    sk_mem_dump("ENGINE-STOP");

    SK_LOG_INFO(core->logger,
             "=================== skull engine stopping ===================");
    sk_print("=================== skull engine stopping ===================\n");

    // 1. stop master
    SK_LOG_INFO(core->logger, "Stopping engine master...");
    sk_engine_stop(core->master);

    // 2. stop workers
    for (int i = 0; i < core->config->threads; i++) {
        SK_LOG_INFO(core->logger, "Stopping engine worker[%d]...", i);
        sk_engine_stop(core->workers[i]);
    }

    // 3. stop bio(s)
    for (int i = 0; i < core->config->bio_cnt; i++) {
        SK_LOG_INFO(core->logger, "Stopping engine bio[%d]...", i + 1);
        sk_engine_stop(core->bio[i]);
    }

    sk_mem_dump("ENGINE-STOP-DONE");
}

void sk_core_destroy(sk_core_t* core) {
    core->status = SK_CORE_DESTROYING;

    sk_mem_dump("ENGINE-DESTROY");

    SK_LOG_INFO(core->logger,
             "================== skull engine destroying ==================");
    sk_print("================== skull engine destroying ==================\n");

    // destroy drivers
    sk_driver_t* driver = NULL;
    while ((driver = flist_pop(core->drivers))) {
        sk_driver_destroy(driver);
    }
    flist_delete(core->drivers);

    _sk_engines_destroy(core);

    _sk_module_destroy(core);

    _sk_service_destroy(core);

    sk_mon_destroy(core->mon);
    sk_mon_destroy(core->umon);

    sk_config_destroy(core->config);

    // destroy workflows
    sk_workflow_t* workflow = NULL;
    while ((workflow = flist_pop(core->workflows))) {
        sk_workflow_destroy(workflow);
    }
    flist_delete(core->workflows);

    _sk_admin_destroy(core);

    free((void*)core->working_dir);

    _sk_destroy_user_loaders(core);

    // destroy loggers
    sk_print("=================== skull engine stopped ====================\n");
    SK_LOG_INFO(core->logger,
             "=================== skull engine stopped ====================");

    sk_logger_destroy(core->logger);
    core->logger = NULL;

    // Destroy env and mem statistics
    sk_mem_dump("ENGINE-DESTROY-DONE");

    sk_logger_destroy(core->logger_diag);
    core->logger_diag = NULL;

    _sk_env_destroy(core);
}

// Utils APIs
sk_service_t* sk_core_service(sk_core_t* core, const char* service_name) {
    SK_ASSERT(core);
    SK_ASSERT(service_name);

    return fhash_str_get(core->services, service_name);
}

sk_core_status_t sk_core_status(sk_core_t* core) { return core->status; }

sk_engine_t* sk_core_bio(sk_core_t* core, int idx) {
    int bidx = idx - 1;

    if (bidx < 0 || bidx >= core->config->bio_cnt) {
        return NULL;
    }

    return core->bio[bidx];
}

const char* sk_core_binpath(sk_core_t* core) {
    return core->cmd_args.binary_path;
}
