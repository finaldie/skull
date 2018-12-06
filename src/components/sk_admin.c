#ifndef  _POSIX_C_SOURCE
# define _POSIX_C_SOURCE 200809L
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <time.h>

#ifdef SKULL_JEMALLOC_LINKED
# include "jemalloc/jemalloc.h"
#endif

#include "flibs/flist.h"
#include "flibs/fmbuf.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_core.h"
#include "api/sk_txn.h"
#include "api/sk_log.h"
#include "api/sk_mon.h"
#include "api/sk_malloc.h"
#include "api/sk_admin.h"

#define ADMIN_CMD_HELP_CONTENT \
    "commands:\n" \
    " - help\n" \
    " - counter | metrics\n" \
    " - last\n" \
    " - info | status\n" \
    " - memory [detail|full|trace=true|trace=false]\n"

#define ADMIN_CMD_HELP            "help"
#define ADMIN_CMD_METRICS         "metrics"
#define ADMIN_CMD_COUNTER         "counter"
#define ADMIN_CMD_LAST_SNAPSHOT   "last"
#define ADMIN_CMD_STATUS          "status"
#define ADMIN_CMD_INFO            "info"
#define ADMIN_CMD_MEMORY          "memory"

#define ADMIN_CMD_MAX_ARGS        (10)
#define ADMIN_CMD_MAX_LENGTH      (256)

#define ADMIN_LINE_MAX_LENGTH     (1024)
#define ADMIN_RESP_MAX_LENGTH     (8192)
#define ADMIN_COUNTER_LINE_LENGTH (256)

#define IFMT(k, fmt)              "%-30s: " fmt, #k

#define MEM_FMT                   "%7s %20s %16s: %zu\n"
#define MEM_FMT_ARGS(t, n, metrics) \
    MEM_FMT, t, n, #metrics, st->metrics

#define MODULE_STAT_FMT           "%s - unpack %zu | run %zu | pack %zu ; "

static const char* LEVELS[] = {
    "Trace", "Debug", "Info", "Warn", "Error", "Fatal"};

static const char* STATUS[] = {
    "Initializing", "Starting", "Running", "Stopping", "Destroying"};

static sk_module_t _sk_admin_module;
static sk_module_cfg_t _sk_admin_module_cfg;

typedef struct sk_admin_data_t {
    int    ignore;
    int    argc;

    char*  command[ADMIN_CMD_MAX_ARGS];
    char   raw[ADMIN_CMD_MAX_LENGTH];
    fmbuf* response;
} sk_admin_data_t;

/******************************** Internal APIs *******************************/
static
sk_admin_data_t* _sk_admin_data_create()
{
    sk_admin_data_t* admin_data = calloc(1, sizeof(*admin_data));
    admin_data->response = fmbuf_create(ADMIN_RESP_MAX_LENGTH);

    return admin_data;
}

static
void _sk_admin_data_destroy(sk_admin_data_t* admin_data)
{
    if (!admin_data) return;

    fmbuf_delete(admin_data->response);
    free(admin_data);
}

static
void _process_help(sk_txn_t* txn)
{
    sk_admin_data_t* admin_data = sk_txn_udata(txn);

    // Populate the response
    fmbuf_push(admin_data->response, ADMIN_CMD_HELP_CONTENT,
               sizeof(ADMIN_CMD_HELP_CONTENT));
}

static
void _mon_cb(const char* name, double value, void* ud)
{
    sk_admin_data_t* admin_data = ud;
    fmbuf* buf = admin_data->response;

    char metrics_str[ADMIN_COUNTER_LINE_LENGTH];
    int printed = snprintf(metrics_str, ADMIN_COUNTER_LINE_LENGTH,
                           "%s: %f\n", name, value);

    if (printed < 0) {
        sk_print("metrics buffer is too small(%d), name: %s, value: %f\n",
                 ADMIN_COUNTER_LINE_LENGTH, name, value);
        SK_LOG_WARN(SK_ENV_LOGGER,
            "metrics buffer is too small(%d), name: %s, value: %f",
            ADMIN_COUNTER_LINE_LENGTH, name, value);
        return;
    }

    int ret = fmbuf_push(buf, metrics_str, (size_t)printed);
    if (!ret) return;

    admin_data->response = fmbuf_realloc(buf, fmbuf_size(buf) + 1024);
    buf = admin_data->response;

    // One more time to fill it again
    if (fmbuf_push(buf, metrics_str, (size_t)printed)) {
        sk_print("error in fill metrics -> name: %s, value: %f\n", name, value);
        SK_LOG_WARN(SK_ENV_LOGGER,
                "error in fill metrics -> name: %s, value: %f", name, value);
    }
}

static
void _transport_mon_snapshot(sk_mon_snapshot_t* snapshot,
                             sk_admin_data_t* admin_data)
{
    sk_mon_snapshot_foreach(snapshot, _mon_cb, admin_data);
}

static
void _fill_data_string(time_t time, sk_admin_data_t* admin_data)
{
    struct tm tm;
    gmtime_r(&time, &tm);

    char time_str[20];
    snprintf(time_str, 20, "%04d:%02d:%02d_%02d:%02d:%02d",
             (tm.tm_year + 1900), tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);

    fmbuf_push(admin_data->response, time_str, 19);
}

static
void _fill_first_line(sk_mon_snapshot_t* snapshot,
                      sk_admin_data_t* admin_data)
{

    time_t start = sk_mon_snapshot_starttime(snapshot);
    time_t end   = sk_mon_snapshot_endtime(snapshot);

    _fill_data_string(start, admin_data);
    fmbuf_push(admin_data->response, " to ", 4);
    _fill_data_string(end, admin_data);
    fmbuf_push(admin_data->response, "\n", 1);
}

static
void _process_metrics(sk_txn_t* txn)
{
    sk_admin_data_t* admin_data = sk_txn_udata(txn);
    sk_core_t* core = SK_ENV_CORE;

    // 1. snapshot global metrics
    sk_mon_snapshot_t* global_snapshot = sk_mon_snapshot(core->mon);
    _fill_first_line(global_snapshot, admin_data);
    _transport_mon_snapshot(global_snapshot, admin_data);
    sk_mon_snapshot_destroy(global_snapshot);

    // 2. snapshot master metrics
    sk_mon_snapshot_t* master_snapshot = sk_mon_snapshot(core->master->mon);
    _transport_mon_snapshot(master_snapshot, admin_data);
    sk_mon_snapshot_destroy(master_snapshot);

    // 3. snapshot workers metrics
    int threads = core->config->threads;
    for (int i = 0; i < threads; i++) {
        sk_engine_t* worker = core->workers[i];
        sk_mon_snapshot_t* worker_snapshot = sk_mon_snapshot(worker->mon);
        _transport_mon_snapshot(worker_snapshot, admin_data);
        sk_mon_snapshot_destroy(worker_snapshot);
    }

    // 4. snapshot bio metrics
    for (int i = 0; i < core->config->bio_cnt; i++) {
        sk_engine_t* bio = core->bio[i];
        sk_mon_snapshot_t* bio_snapshot = sk_mon_snapshot(bio->mon);
        _transport_mon_snapshot(bio_snapshot, admin_data);
        sk_mon_snapshot_destroy(bio_snapshot);
    }

    // 5. shapshot user metrics
    sk_mon_snapshot_t* user_snapshot = sk_mon_snapshot(core->umon);
    _transport_mon_snapshot(user_snapshot, admin_data);
    sk_mon_snapshot_destroy(user_snapshot);
}

static
void _process_last_snapshot(sk_txn_t* txn)
{
    sk_admin_data_t* admin_data = sk_txn_udata(txn);
    sk_core_t* core = SK_ENV_CORE;

    // 1. snapshot global metrics
    sk_mon_snapshot_t* global_snapshot = sk_mon_snapshot_latest(core->mon);
    if (!global_snapshot) {
        return;
    }

    _fill_first_line(global_snapshot, admin_data);
    _transport_mon_snapshot(global_snapshot, admin_data);

    // 2. snapshot master metrics
    sk_mon_t* master_mon = core->master->mon;
    sk_mon_snapshot_t* master_snapshot = sk_mon_snapshot_latest(master_mon);
    _transport_mon_snapshot(master_snapshot, admin_data);

    // 3. snapshot workers metrics
    int threads = core->config->threads;
    for (int i = 0; i < threads; i++) {
        sk_engine_t* worker = core->workers[i];
        sk_mon_snapshot_t* worker_snapshot = sk_mon_snapshot_latest(worker->mon);
        _transport_mon_snapshot(worker_snapshot, admin_data);
    }

    // 4. shapshot user metrics
    sk_mon_snapshot_t* user_snapshot = sk_mon_snapshot_latest(core->umon);
    _transport_mon_snapshot(user_snapshot, admin_data);
}

static
void _append_response(sk_txn_t* txn, const char* fmt, ...)
{
    sk_admin_data_t* admin_data = sk_txn_udata(txn);
    char line[ADMIN_LINE_MAX_LENGTH];
    memset(line, 0, sizeof(line));
    int len = 0;

    va_list args;
    va_start(args, fmt);
    len = vsnprintf(line, ADMIN_LINE_MAX_LENGTH, fmt, args);
    va_end(args);

    fmbuf_push(admin_data->response, line, (size_t)len);
}

static
void _status_workflow(sk_txn_t* txn, sk_core_t* core)
{
    flist_iter iter = flist_new_iter(core->workflows);
    sk_workflow_t* workflow = NULL;
    int total = 0;

    while ((workflow = flist_each(&iter))) {
        total++;
    }

    _append_response(txn, IFMT(workflows, "%d\n"), total);
}


static
void _status_module(sk_txn_t* txn, sk_core_t* core)
{
    {
        fhash_str_iter iter = fhash_str_iter_new(core->unique_modules);
        sk_module_t* module = NULL;
        int total = 0;

        while ((module = fhash_str_next(&iter))) {
            total++;
        }

        fhash_str_iter_release(&iter);

        _append_response(txn, IFMT(modules, "%d\n"), total);
    }

    {
        _append_response(txn, IFMT(module_stat, ""));

        fhash_str_iter iter = fhash_str_iter_new(core->unique_modules);
        sk_module_t* module = NULL;

        while ((module = fhash_str_next(&iter))) {
            size_t unpacking_cnt = module->stat.unpack;
            size_t running_cnt   = module->stat.run;
            size_t packing_cnt   = module->stat.pack;
            const char* name     = module->cfg->name;

            _append_response(txn, MODULE_STAT_FMT, name,
                    unpacking_cnt, running_cnt, packing_cnt);
        }

        fhash_str_iter_release(&iter);
        _append_response(txn, "\n");
    }

}

static
void _status_service(sk_txn_t* txn, sk_core_t* core)
{
    {
        fhash_str_iter iter = fhash_str_iter_new(core->services);
        sk_service_t* service = NULL;
        int total = 0;

        while ((service = fhash_str_next(&iter))) {
            total++;
        }

        fhash_str_iter_release(&iter);

        _append_response(txn, IFMT(services, "%d\n"), total);
    }

    {
        _append_response(txn, IFMT(service_stat, ""));

        fhash_str_iter iter = fhash_str_iter_new(core->services);
        sk_service_t* service = NULL;

        while ((service = fhash_str_next(&iter))) {
            _append_response(txn, "%s running_tasks: %d ; ",
                             sk_service_name(service),
                             sk_service_running_taskcnt(service));
        }

        fhash_str_iter_release(&iter);
        _append_response(txn, "\n");
    }
}

static
void _merge_stat(sk_entity_mgr_stat_t* stat, const sk_entity_mgr_stat_t* merging)
{
    stat->total               += merging->total;
    stat->inactive            += merging->inactive;
    stat->entity_none         += merging->entity_none;
    stat->entity_sock_v4tcp   += merging->entity_sock_v4tcp;
    stat->entity_sock_v4udp   += merging->entity_sock_v4udp;
    stat->entity_sock_v6tcp   += merging->entity_sock_v6tcp;
    stat->entity_sock_v6udp   += merging->entity_sock_v6udp;
    stat->entity_timer        += merging->entity_timer;
    stat->entity_ep_v4tcp     += merging->entity_ep_v4tcp;
    stat->entity_ep_v4udp     += merging->entity_ep_v4udp;
    stat->entity_ep_v6tcp     += merging->entity_ep_v6tcp;
    stat->entity_ep_v6udp     += merging->entity_ep_v6udp;
    stat->entity_ep_txn_timer += merging->entity_ep_txn_timer;
}

static
void _status_entity(sk_txn_t* txn, sk_core_t* core)
{
    sk_entity_mgr_stat_t stat = sk_entity_mgr_stat(core->master->entity_mgr);

    for (int i = 0; i < core->config->threads; i++) {
        sk_entity_mgr_stat_t tmp = sk_entity_mgr_stat(core->workers[i]->entity_mgr);
        _merge_stat(&stat, &tmp);
    }

    for (int i = 0; i < core->config->bio_cnt; i++) {
        sk_entity_mgr_stat_t tmp = sk_entity_mgr_stat(core->bio[i]->entity_mgr);
        _merge_stat(&stat, &tmp);
    }

    _append_response(txn, IFMT(entities, "total: %d inactive: %d ; none: %d "
        "std: %d v4tcp: %d v4udp: %d v6tcp: %d v6udp: %d timer: %d "
        "ep_v4tcp: %d ep_v4udp: %d ep_v6tcp: %d ep_v6udp: %d "
        "ep_timer: %d ep_txn_timer: %d\n"),
        stat.total, stat.inactive, stat.entity_none, stat.entity_std,
        stat.entity_sock_v4tcp, stat.entity_sock_v4udp,
        stat.entity_sock_v6tcp, stat.entity_sock_v6udp,
        stat.entity_timer, stat.entity_ep_v4tcp, stat.entity_ep_v4udp,
        stat.entity_ep_v6tcp, stat.entity_ep_v6udp,
        stat.entity_ep_timer, stat.entity_ep_txn_timer);
}

static
void _status_resources(sk_txn_t* txn, sk_core_t* core)
{
    float cpu_sys  = (float)core->info.self_ru.ru_stime.tv_sec +
                     (float)core->info.self_ru.ru_stime.tv_usec / 1000000;
    float cpu_user = (float)core->info.self_ru.ru_utime.tv_sec +
                     (float)core->info.self_ru.ru_utime.tv_usec / 1000000;

    float prev_cpu_sys  = (float)core->info.prev_self_ru.ru_stime.tv_sec +
                          (float)core->info.prev_self_ru.ru_stime.tv_usec / 1000000;
    float prev_cpu_user = (float)core->info.prev_self_ru.ru_utime.tv_sec +
                          (float)core->info.prev_self_ru.ru_utime.tv_usec / 1000000;

    float last_cpu_user = cpu_user - prev_cpu_user;
    float last_cpu_sys  = cpu_sys  - prev_cpu_sys;
    last_cpu_sys        = last_cpu_sys > 0.000001f ? last_cpu_sys : 0.000001f;

    _append_response(txn, IFMT(cpu_user, "%.2f\n"), last_cpu_user);
    _append_response(txn, IFMT(cpu_sys, "%.2f\n"),  last_cpu_sys);
    _append_response(txn, IFMT(cpu_user/sys, "%.2f\n"), last_cpu_user / last_cpu_sys);

    _append_response(txn, IFMT(memory_rss(KB), "%zu\n"), core->info.self_ru.ru_maxrss);
    _append_response(txn, IFMT(swaps, "%ld\n"), core->info.self_ru.ru_nswap);
    _append_response(txn, IFMT(page_faults, "%ld\n"), core->info.self_ru.ru_majflt);
    _append_response(txn, IFMT(block_input, "%ld\n"), core->info.self_ru.ru_inblock);
    _append_response(txn, IFMT(block_output, "%ld\n"), core->info.self_ru.ru_oublock);
    _append_response(txn, IFMT(signals, "%ld\n"), core->info.self_ru.ru_nsignals);
    _append_response(txn, IFMT(voluntary_context_switches, "%ld\n"), core->info.self_ru.ru_nvcsw);
    _append_response(txn, IFMT(involuntary_context_switches, "%ld\n"), core->info.self_ru.ru_nivcsw);

}

static
void _process_status(sk_txn_t* txn)
{
    sk_core_t* core = SK_ENV_CORE;

    _append_response(txn, IFMT(version, "%s\n"), core->info.version);
    _append_response(txn, IFMT(git_sha1, "%s\n"), core->info.git_sha1);

    _append_response(txn, IFMT(compiler, "%s %s\n"),
                     core->info.compiler, core->info.compiler_version);
    _append_response(txn, IFMT(compiling_options, "%s\n"),
                     core->info.compiling_options);
    _append_response(txn, "\n");

    _append_response(txn, IFMT(daemon, "%d\n"), core->cmd_args.daemon);
    _append_response(txn, IFMT(log_std_forward, "%d\n"),
                     core->cmd_args.log_stdout_fwd);
    _append_response(txn, IFMT(log_rolling, "%d\n"),
                     !core->cmd_args.log_rolling_disabled);
    _append_response(txn, IFMT(max_open_files, "%d\n"), core->max_fds);

    // config: config file absolute location
    _append_response(txn, IFMT(binary, "%s\n"), core->cmd_args.binary_path);
    _append_response(txn, IFMT(working_dir,    "%s\n"), core->working_dir);
    _append_response(txn, IFMT(configuration,  "%s\n"), core->config->location);
    _append_response(txn, IFMT(worker_threads, "%d\n"), core->config->threads);
    _append_response(txn, IFMT(bio_threads,    "%d\n"), core->config->bio_cnt);
    _append_response(txn, "\n");

    // config: log file absolute location
    int log_level = core->config->log_level;
    _append_response(txn, IFMT(log_level, "%s\n"), LEVELS[log_level]);
    _append_response(txn, IFMT(log_file,  "%s/log/%s\n"),
                     core->working_dir, core->config->log_name);
    _append_response(txn, IFMT(diag_file, "%s/log/%s\n"),
                     core->working_dir, core->config->diag_name);

    if (core->cmd_args.daemon) {
        _append_response(txn, IFMT(stdout, "%s/log/%s\n"),
                     core->working_dir, "stdout.log");
        _append_response(txn, IFMT(stderr, "%s/log/%s\n"),
                     core->working_dir, "stdout.log");
    } else {
        _append_response(txn, IFMT(stdout, "/proc/%d/fd/1\n"), core->info.pid);
        _append_response(txn, IFMT(stderr, "/proc/%d/fd/2\n"), core->info.pid);
    }
    _append_response(txn, "\n");

    _append_response(txn, IFMT(pid, "%d\n"), core->info.pid);
    _append_response(txn, IFMT(uptime(s), "%ld\n"), time(NULL) - core->starttime);
    _append_response(txn, IFMT(status, "%s\n"), STATUS[sk_core_status(core)]);
    _append_response(txn, "\n");

    _status_resources(txn, core);
    _append_response(txn, "\n");

    _status_workflow(txn, core);
    _status_module(txn, core);
    _status_service(txn, core);
    _status_entity(txn, core);
}

static
void __append_jemalloc_stat(void* txn, const char * content)
{
    _append_response(txn, "%s", content);
}

static
void __append_mem_stat(sk_txn_t* txn,
                       const char* t, // tag
                       const char* n, // name
                       const sk_mem_stat_t* st,
                       bool detail)
{
    _append_response(txn, MEM_FMT, t, n, "used", sk_mem_allocated(st));
    if (!detail) return;

    _append_response(txn, MEM_FMT_ARGS(t, n, nmalloc));
    _append_response(txn, MEM_FMT_ARGS(t, n, nfree));
    _append_response(txn, MEM_FMT_ARGS(t, n, ncalloc));
    _append_response(txn, MEM_FMT_ARGS(t, n, nrealloc));
    _append_response(txn, MEM_FMT_ARGS(t, n, nposix_memalign));
    _append_response(txn, MEM_FMT_ARGS(t, n, naligned_alloc));
    _append_response(txn, MEM_FMT_ARGS(t, n, alloc_sz));
    _append_response(txn, MEM_FMT_ARGS(t, n, dalloc_sz));
    _append_response(txn, MEM_FMT_ARGS(t, n, peak_sz));
    _append_response(txn, "\n");
}

static
void _process_memory_info(sk_txn_t* txn, bool detail) {
    // 1. Dump skull core mem stat
    sk_core_t* core = SK_ENV_CORE;
    size_t total_allocated = 0;

    const sk_mem_stat_t* static_stat = sk_mem_stat_static();
    const sk_mem_stat_t* core_stat   = sk_mem_stat_core();

    _append_response(txn, "========== Skull Memory Measurement ==========\n");
    __append_mem_stat(txn, ".", "init", static_stat, detail);
    total_allocated += sk_mem_allocated(static_stat);

    __append_mem_stat(txn, ".", "core", core_stat, detail);
    total_allocated += sk_mem_allocated(core_stat);

    const char* admin_name = sk_admin_module()->cfg->name;
    const sk_mem_stat_t* admin_st = sk_mem_stat_module(admin_name);
    __append_mem_stat(txn, "module", admin_name, admin_st, detail);
    total_allocated += sk_mem_allocated(admin_st);

    {
        fhash_str_iter iter = fhash_str_iter_new(core->unique_modules);
        sk_module_t* module = NULL;

        while ((module = fhash_str_next(&iter))) {
            const sk_mem_stat_t* stat = sk_mem_stat_module(module->cfg->name);

            __append_mem_stat(txn, "module", module->cfg->name, stat, detail);
            total_allocated += sk_mem_allocated(stat);
        }

        fhash_str_iter_release(&iter);
    }

    {
        fhash_str_iter iter = fhash_str_iter_new(core->services);
        sk_service_t* service = NULL;

        while ((service = fhash_str_next(&iter))) {
            const char* service_name = sk_service_name(service);
            const sk_mem_stat_t* stat = sk_mem_stat_service(service_name);

            __append_mem_stat(txn, "service", service_name, stat, detail);
            total_allocated += sk_mem_allocated(stat);
        }

        fhash_str_iter_release(&iter);
    }

    _append_response(txn, "\n================== Summary ===================\n");
    _append_response(txn, "Total Allocated: %zu\n", total_allocated);
    _append_response(txn, "Trace Enabled: %d\n\n", sk_mem_trace_status());
}

static
void _process_memory_allocator(sk_txn_t* txn) {
#   ifdef SKULL_JEMALLOC_LINKED
    je_malloc_stats_print(__append_jemalloc_stat, txn, NULL);
#   else
    _append_response(txn, "Jemalloc Stat Not Available\r\n");
#   endif
}

static
void _process_memory(sk_admin_data_t* admin_data, sk_txn_t* txn)
{
    const char* subcommand = NULL;
    if (admin_data->argc == 1) {
        subcommand = "info";
    } else {
        subcommand = admin_data->command[1];
    }

    if (0 == strcasecmp("info", subcommand)) {
        _process_memory_info(txn, false);
    } else if (0 == strcasecmp("detail", subcommand)) {
        _process_memory_info(txn, true);
    } else if (0 == strcasecmp("trace=true", subcommand)) {
        sk_mem_trace(true);
        _process_memory_info(txn, false);
    } else if (0 == strcasecmp("trace=false", subcommand)) {
        sk_mem_trace(false);
        _process_memory_info(txn, false);
    } else if (0 == strcasecmp("full", subcommand)) {
        _process_memory_info(txn, true);
        _process_memory_allocator(txn);
    } else {
        _process_help(txn);
    }
}

/********************************* Public APIs ********************************/
sk_workflow_cfg_t* sk_admin_workflowcfg_create(const char* bind, int port)
{
    sk_workflow_cfg_t* cfg = calloc(1, sizeof(*cfg));
    cfg->concurrent   = 0;
    cfg->enable_stdin = 0;
    cfg->port         = port;
    cfg->bind         = bind ? bind : "127.0.0.1";
    cfg->idl_name     = NULL;
    cfg->modules      = flist_create();

    // To prevent Admin module realloc txn->output buffer which nosiy the memory
    //  measurement. Here from the observation, use 8192 would be a big enough
    //  number, we can adjust it when necessary.
    cfg->txn_out_sz   = ADMIN_RESP_MAX_LENGTH;

    flist_push(cfg->modules, &_sk_admin_module_cfg);

    return cfg;
}

void sk_admin_workflowcfg_destroy(sk_workflow_cfg_t* cfg)
{
    if (!cfg) return;

    flist_delete(cfg->modules);
    free(cfg);
}

sk_module_t* sk_admin_module()
{
    return &_sk_admin_module;
}

/****************************** Admin Module **********************************/
static
int  _admin_init(void* md)
{
    sk_print("admin module: init\n");
    return 0;
}

static
void _admin_release(void* md)
{
    sk_print("admin module: release\n");
}

static
int _admin_run(void* md, sk_txn_t* txn)
{
    sk_print("admin module: run\n");

    sk_admin_data_t* admin_data = sk_txn_udata(txn);
    bool ignore = admin_data->ignore;
    if (ignore) {
        return 0;
    }

    const char* command = admin_data->command[0];
    sk_print("Receive command: %s\n", command);

    if (0 == strcasecmp(ADMIN_CMD_HELP, command)) {
        _process_help(txn);
    } else if (0 == strcasecmp(ADMIN_CMD_METRICS, command) ||
               0 == strcasecmp(ADMIN_CMD_COUNTER, command)) {
        _process_metrics(txn);
    } else if (0 == strcasecmp(ADMIN_CMD_LAST_SNAPSHOT, command)) {
        _process_last_snapshot(txn);
    } else if (0 == strcasecmp(ADMIN_CMD_STATUS, command) ||
               0 == strcasecmp(ADMIN_CMD_INFO, command)) {
        _process_status(txn);
    } else if (0 == strcasecmp(ADMIN_CMD_MEMORY, command)) {
        _process_memory(admin_data, txn);
    } else {
        _process_help(txn);
    }

    return 0;
}

static
ssize_t _admin_unpack(void* md, struct sk_txn_t* txn,
                      const void* data, size_t data_sz)
{
    sk_print("admin_unpack, data sz: %zu\n", data_sz);
    SK_LOG_DEBUG(SK_ENV_LOGGER,
                "admin module_unpack: data sz:%zu", data_sz);

    const char* cmd = data;
    if (0 != strncmp(&cmd[data_sz - 1], "\n", 1)) {
        return 0;
    }

    size_t tailer_offset = 1;
    if (data_sz >= 2 && 0 == strncmp(&cmd[data_sz - 2], "\r", 1)) {
        tailer_offset += 1;
    }

    int ignore = data_sz - tailer_offset ? 0 : 1;

    sk_admin_data_t* admin_data = _sk_admin_data_create();
    // for the empty line, just mark this request can be ignored
    if (ignore) {
        admin_data->ignore = ignore;
        goto unpack_done;
    }

    // store command string, remove the '\r\n', and append '\0'
    size_t max_len = SK_MIN(data_sz - tailer_offset, ADMIN_CMD_MAX_LENGTH - 1);
    strncpy(admin_data->raw, data, max_len);

    sk_print("Receive raw command: %s\n", admin_data->raw);
    SK_LOG_INFO(SK_ENV_LOGGER, "Received command: %s", admin_data->raw);

    for (char* token = NULL, *cmds = admin_data->raw;
         admin_data->argc < ADMIN_CMD_MAX_ARGS &&
         (token = strsep(&cmds, " ")) != NULL;
         admin_data->argc++) {
        admin_data->command[admin_data->argc] = token;
    }

unpack_done:
    sk_txn_setudata(txn, admin_data);

    return (ssize_t)data_sz;
}

static
int  _admin_pack(void* md, struct sk_txn_t* txn)
{
    sk_print("admin_pack\n");

    sk_admin_data_t* admin_data = sk_txn_udata(txn);
    if (admin_data->ignore) {
        goto pack_done;
    }

    // send the data to output buffer
    const char* response = fmbuf_head(admin_data->response);
    size_t response_sz   = fmbuf_used(admin_data->response);
    sk_txn_output_append(txn, response, response_sz);

pack_done:
    _sk_admin_data_destroy(admin_data);
    return 0;
}

static
sk_module_cfg_t _sk_admin_module_cfg = {
    .name = "admin",
    .type = "built-in"
};

static
sk_module_t _sk_admin_module = {
    .md   = NULL,
    .cfg  = &_sk_admin_module_cfg,

    .init    = _admin_init,
    .run     = _admin_run,
    .unpack  = _admin_unpack,
    .pack    = _admin_pack,
    .release = _admin_release
};

