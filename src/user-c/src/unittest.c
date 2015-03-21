#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <google/protobuf-c/protobuf-c.h>

#include "api/sk_utils.h"
#include "api/sk_env.h"
#include "api/sk_module.h"
#include "api/sk_workflow.h"
#include "api/sk_entity.h"
#include "api/sk_txn.h"
#include "api/sk_service.h"
#include "api/sk_loader.h"

#include "skull/idl.h"
#include "skull/metrics_utils.h"
#include "idl_internal.h"

#include "skull/unittest.h"

struct sk_service_t {
    sk_service_type_t type;
    int running_task_cnt;

    const char*       name; // a ref
    sk_service_opt_t  opt;
    const sk_service_cfg_t* cfg;

    sk_queue_t* pending_tasks; // This queue is only avaliable in master
    sk_srv_data_t* data;

    uint64_t last_taskid;
};

struct skull_utenv_t {
    sk_module_t*    module;
    sk_service_t**  service;
    size_t          service_cnt;

    sk_workflow_t*  workflow;
    sk_entity_t*    entity;
    sk_txn_t*       txn;

    sk_workflow_cfg_t* workflow_cfg;
};

skull_utenv_t* skull_utenv_create(const char* module_name,
                                  const char* idl_name,
                                  const char* conf_name)
{
    skull_utenv_t* env = calloc(1, sizeof(*env));
    env->module = sk_module_load(module_name, conf_name);

    env->workflow_cfg = calloc(1, sizeof(sk_workflow_cfg_t));
    env->workflow_cfg->idl_name = idl_name;
    env->workflow_cfg->port = SK_CONFIG_NO_PORT;
    env->workflow = sk_workflow_create(env->workflow_cfg);
    env->entity = sk_entity_create(env->workflow);
    env->txn = sk_txn_create(env->workflow, env->entity);

    // run init
    env->module->init(env->module->md);

    return env;
}

void skull_utenv_destroy(skull_utenv_t* env)
{
    if (!env) {
        return;
    }

    env->module->release(env->module->md);
    sk_module_unload(env->module);

    skull_idl_data_t* idl_data = sk_txn_udata(env->txn);
    free(idl_data->data);
    free(idl_data);
    sk_txn_destroy(env->txn);

    sk_entity_destroy(env->entity);
    sk_workflow_destroy(env->workflow);
    free(env->workflow_cfg);
    free(env);
}

int skull_utenv_add_service(skull_utenv_t* env, const char* name)
{
    SK_ASSERT_MSG(name, "service name must not empty\n");

    size_t service_cnt = env->service_cnt + 1;
    env->service =
        realloc(env->service, service_cnt * sizeof(sk_service_t));

    env->service[env->service_cnt]->name = name;
    env->service_cnt = service_cnt;

    return 0;
}

int skull_utenv_run(skull_utenv_t* env, bool run_unapck, bool run_pack)
{
    int ret = env->module->run(env->module->md, env->txn);
    return ret;
}

void* skull_utenv_sharedata(skull_utenv_t* env)
{
    const ProtobufCMessageDescriptor* desc =
        skull_idl_descriptor(env->workflow_cfg->idl_name);

    ProtobufCMessage* msg = NULL;
    skull_idl_data_t* idl_data = sk_txn_udata(env->txn);
    if (idl_data) {
        msg = protobuf_c_message_unpack(
            desc, NULL, idl_data->data_sz, idl_data->data);
    } else {
        msg = calloc(1, desc->sizeof_message);
        protobuf_c_message_init(desc, msg);
    }

    return msg;
}

void  skull_utenv_sharedata_release(void* data)
{
    if (!data) {
        return;
    }

    protobuf_c_message_free_unpacked(data, NULL);
}

void  skull_utenv_sharedata_reset(skull_utenv_t* env, const void* msg)
{
    sk_txn_t* txn = env->txn;
    skull_idl_data_t* idl_data = sk_txn_udata(txn);
    if (!msg) {
        if (idl_data) {
            free(idl_data->data);
        }

        return;
    }

    size_t new_msg_sz = protobuf_c_message_get_packed_size(msg);
    void*  new_msg_data = calloc(1, new_msg_sz);
    size_t packed_sz = protobuf_c_message_pack(msg, new_msg_data);
    SK_ASSERT(new_msg_sz == packed_sz);

    if (!idl_data) {
        idl_data = calloc(1, sizeof(*idl_data));
        idl_data->data = new_msg_data;
        idl_data->data_sz = packed_sz;
        sk_txn_setudata(txn, idl_data);
    } else {
        free(idl_data->data);

        idl_data->data = new_msg_data;
        idl_data->data_sz = packed_sz;
    }
}

// Mock API for skull_txn (no needed, link the txn.o)

// Mock API for skull_log
//  Redirect the logger to stdout
void skull_log(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

bool skull_log_enable_trace() { return true; }
bool skull_log_enable_debug() { return true; }
bool skull_log_enable_info()  { return true; }
bool skull_log_enable_warn()  { return true; }
bool skull_log_enable_error() { return true; }
bool skull_log_enable_fatal() { return true; }

const char* skull_log_info_msg(int log_id) { return "fake info message"; }
const char* skull_log_warn_msg(int log_id) { return "fake warn message"; }
const char* skull_log_warn_solution(int log_id) { return "fake warn solution"; }
const char* skull_log_error_msg(int log_id) { return "fake error message"; }
const char* skull_log_error_solution(int log_id) { return "fake error solution"; }
const char* skull_log_fatal_msg(int log_id) { return "fake fatal message"; }
const char* skull_log_fatal_solution(int log_id) { return "fake fatal solution"; }

// Mock API for skull_metrics
void skull_metric_inc(const char* name, double value)
{
    // No implementation, note: test metrics in FT
}

double skull_metric_get(const char* name)
{
    // No implementation, note: test metrics in FT
    return 0.0f;
}

void skull_metric_foreach(skull_metric_each metric_cb, void* ud)
{
    // No implementation, note: test metrics in FT
}

// Mock API for skull_idl (no needed, link idl.o)

// Mock API for sk_service
const char* sk_service_name(const sk_service_t* service)
{
    return service->name;
}

void sk_service_setopt(sk_service_t* service, sk_service_opt_t opt)
{
    service->opt = opt;
}

void sk_service_settype(sk_service_t* service, sk_service_type_t type)
{
    service->type = type;
}

sk_service_type_t sk_service_type(const sk_service_t* service)
{
    return service->type;
}
