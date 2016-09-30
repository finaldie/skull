#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "flibs/flist.h"
#include "flibs/fmbuf.h"

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_core.h"
#include "api/sk_txn.h"
#include "api/sk_log.h"
#include "api/sk_mon.h"
#include "api/sk_admin.h"

#define ADMIN_CMD_HELP_CONTENT \
    "commands:\n - help\n - metrics\n - last\n - status\n"

#define ADMIN_CMD_HELP          "help"
#define ADMIN_CMD_METRICS       "metrics"
#define ADMIN_CMD_LAST_SNAPSHOT "last"
#define ADMIN_CMD_STATUS        "status"

static sk_module_t _sk_admin_module;
static sk_module_cfg_t _sk_admin_module_cfg;

typedef struct sk_admin_data_t {
    int   ignore;

#if __WORDSIZE == 64
    int   _padding;
#endif

    char*  command;
    fmbuf* response;
} sk_admin_data_t;

/******************************** Internal APIs *******************************/
static
sk_admin_data_t* _sk_admin_data_create()
{
    sk_admin_data_t* admin_data = calloc(1, sizeof(*admin_data));
    admin_data->response = fmbuf_create(1024);

    return admin_data;
}

static
void _sk_admin_data_destroy(sk_admin_data_t* admin_data)
{
    if (!admin_data) return;

    free(admin_data->command);
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

    char metrics_str[128];
    int printed = snprintf(metrics_str, 128, "%s: %f\n", name, value);

    if (printed < 0) {
        sk_print("metrics buffer is too small(128), name: %s, value: %f\n",
                 name, value);
        SK_LOG_WARN(SK_ENV_LOGGER,
            "metrics buffer is too small(128), name: %s, value: %f",
            name, value);
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
void _process_status(sk_txn_t* txn)
{
    sk_admin_data_t* admin_data = sk_txn_udata(txn);
    sk_core_t* core = SK_ENV_CORE;

    sk_core_status_t status = sk_core_status(core);
    char status_str[3];
    memset(status_str, 0, sizeof(status_str));
    snprintf(status_str, 3, "%d\n", status);

    fmbuf_push(admin_data->response, status_str, 2);
}

/********************************* Public APIs ********************************/
sk_workflow_cfg_t* sk_admin_workflowcfg_create(int port)
{
    sk_workflow_cfg_t* cfg = calloc(1, sizeof(*cfg));
    cfg->concurrent   = 0;
    cfg->enable_stdin = 0;
    cfg->port         = port;
    cfg->bind4        = "127.0.0.1";
    cfg->idl_name     = NULL;
    cfg->modules      = flist_create();

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
void _admin_init(void* md)
{
    sk_print("admin module: init\n");
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

    const char* command = admin_data->command;
    sk_print("receive command: %s\n", command);
    SK_LOG_INFO(SK_ENV_LOGGER, "receive command: %s", command);

    if (0 == strcasecmp(ADMIN_CMD_HELP, command)) {
        _process_help(txn);
    } else if (0 == strcasecmp(ADMIN_CMD_METRICS, command)) {
        _process_metrics(txn);
    } else if (0 == strcasecmp(ADMIN_CMD_LAST_SNAPSHOT, command)) {
        _process_last_snapshot(txn);
    } else if (0 == strcasecmp(ADMIN_CMD_STATUS, command)) {
        _process_status(txn);
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
    SK_LOG_INFO(SK_ENV_LOGGER,
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
    admin_data->command = calloc(1, data_sz - tailer_offset + 1);
    memcpy(admin_data->command, data, data_sz - tailer_offset);

unpack_done:
    sk_txn_setudata(txn, admin_data);

    return (ssize_t)data_sz;
}

static
void _admin_pack(void* md, struct sk_txn_t* txn)
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
}

static
sk_module_cfg_t _sk_admin_module_cfg = {
    .name = "Admin",
    .type = "Native"
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
