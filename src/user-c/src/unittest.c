#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <google/protobuf-c/protobuf-c.h>

#include "flibs/fhash.h"
#include "flibs/flist.h"

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
#include "txn_types.h"
#include "srv_types.h"
#include "srv_loader.h"

#include "skull/unittest.h"

typedef struct mock_service_t {
    const char* name;

    skull_service_async_api_t** apis;
} mock_service_t;

typedef struct mock_task_t {
    const void* req_msg;  // This is a protobuf message
    mock_service_t* service;
    skull_service_async_api_t* api;
    skull_module_cb cb;
} mock_task_t;

struct skullut_module_t {
    sk_module_t*    module;
    fhash*          services; // key: name; value: mock_service

    sk_workflow_t*  workflow;
    sk_entity_t*    entity;
    sk_txn_t*       txn;
    flist*          tasks;    // hold mock_tasks

    sk_workflow_cfg_t* workflow_cfg;
};

static
skull_service_async_api_t*
_find_api(skull_service_async_api_t** apis, const char* api_name)
{
    if (!apis) {
        return NULL;
    }

    skull_service_async_api_t* api = apis[0];
    for (; api != NULL; api += 1) {
        if (0 == strcmp(api->name, api_name)) {
            break;
        }
    }

    return api;
}

skullut_module_t* skullut_module_create(const char* module_name,
                                  const char* idl_name,
                                  const char* conf_name)
{
    skullut_module_t* env = calloc(1, sizeof(*env));
    env->module = sk_module_load(module_name, conf_name);

    env->workflow_cfg = calloc(1, sizeof(sk_workflow_cfg_t));
    env->workflow_cfg->idl_name = idl_name;
    env->workflow_cfg->port = SK_CONFIG_NO_PORT;
    env->workflow = sk_workflow_create(env->workflow_cfg);

    env->services = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    // create a hacked entity, set 'utenv' into half_txn field, we won't use
    // half_txn in UT env, so it's safe for us to use it
    env->entity = sk_entity_create(env->workflow);
    sk_entity_sethalftxn(env->entity, (void*)env);

    env->txn = sk_txn_create(env->workflow, env->entity);
    env->tasks = flist_create();

    // run init
    env->module->init(env->module->md);

    return env;
}

void skullut_module_destroy(skullut_module_t* env)
{
    if (!env) {
        return;
    }

    env->module->release(env->module->md);
    sk_module_unload(env->module);

    skull_idl_data_t* idl_data = sk_txn_udata(env->txn);
    if (idl_data) {
        free(idl_data->data);
        free(idl_data);
    }

    sk_txn_destroy(env->txn);

    sk_entity_destroy(env->entity);
    sk_workflow_destroy(env->workflow);

    // destroy the services
    fhash_str_iter srv_iter = fhash_str_iter_new(env->services);
    mock_service_t* service = NULL;

    while ((service = fhash_str_next(&srv_iter))) {
        free(service);
    }

    fhash_str_iter_release(&srv_iter);
    fhash_str_delete(env->services);
    flist_delete(env->tasks);

    free(env->workflow_cfg);
    free(env);
}

int skullut_module_mocksrv_add(skullut_module_t* env, const char* name,
                            skull_service_async_api_t** apis,
                            const ProtobufCMessageDescriptor** tbl)
{
    SK_ASSERT_MSG(name, "service name must not empty\n");

    mock_service_t* service = calloc(1, sizeof(*service));
    service->name = name;
    service->apis = apis;

    fhash_str_set(env->services, name, service);

    skull_srv_idl_register(tbl);

    return 0;
}

int skullut_module_run(skullut_module_t* env)
{
    int ret = env->module->run(env->module->md, env->txn);

    if (flist_empty(env->tasks)) {
        return ret;
    }

    // process all service tasks
    mock_task_t* task = NULL;
    while ((task = flist_pop(env->tasks))) {
        // 1. prepare a response
        skull_service_async_api_t* api = task->api;

        char resp_proto_name[SKULL_SRV_PROTO_MAXLEN];
        snprintf(resp_proto_name, SKULL_SRV_PROTO_MAXLEN, "%s.%s_resp",
                 task->service->name, api->name);

        const ProtobufCMessageDescriptor* resp_desc =
            skull_srv_idl_descriptor(resp_proto_name);
        SK_ASSERT(resp_desc);

        ProtobufCMessage* resp_msg = NULL;
        resp_msg = calloc(1, resp_desc->sizeof_message);
        protobuf_c_message_init(resp_desc, resp_msg);

        // 2. call api
        skull_service_t skull_service = {
            .service = (sk_service_t*)task->service
        };

        api->iocall(&skull_service, task->req_msg, resp_msg);

        // 3. call module callback
        skull_txn_t skull_txn = {
            .txn = env->txn
        };

        task->cb(&skull_txn, task->req_msg, resp_msg);

        // 4. clean up
        // notes: the req_msg no needs to be released, due to it's on the stack
        protobuf_c_message_free_unpacked(resp_msg, NULL);
        free(task);
    }

    return ret;
}

void* skullut_module_data(skullut_module_t* env)
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

void  skullut_module_data_release(void* data)
{
    if (!data) {
        return;
    }

    protobuf_c_message_free_unpacked(data, NULL);
}

void  skullut_module_data_reset(skullut_module_t* env, const void* msg)
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

void sk_mon_inc(sk_mon_t* sk_mon, const char* name, double value)
{
    // No implementation, note: test metrics in FT
}

double sk_mon_get(sk_mon_t* sk_mon, const char* name)
{
    // No implementation, note: test metrics in FT
    return 0.0f;
}

// Fake core
static sk_core_t _fake_core = {
    .mon = NULL
};

// Fake engine
static sk_engine_t _fake_engine = {
    .mon = NULL
};

// Fake thread_env
static sk_thread_env_t _fake_env = {
    .core   = &_fake_core,
    .engine = &_fake_engine,
    .name   = "fake_thread"
};

// sk env
sk_thread_env_t* sk_thread_env()
{
    return &_fake_env;
}

// Mock API for running mock service api in module ut
skull_service_ret_t
skull_service_async_call (skull_txn_t* txn,
                          const char* service_name,
                          const char* api_name,
                          const void* request,
                          skull_module_cb cb,
                          int bidx)
{
    if (!service_name) {
        return SKULL_SERVICE_ERROR_SRVNAME;
    }

    sk_txn_t* sk_txn = txn->txn;
    sk_entity_t* entity = sk_txn_entity(sk_txn);
    skullut_module_t* env = (skullut_module_t*)sk_entity_halftxn(entity);

    mock_service_t* service = fhash_str_get(env->services, service_name);
    if (!service) {
        return SKULL_SERVICE_ERROR_SRVNAME;
    }

    skull_service_async_api_t* api = _find_api(service->apis, api_name);
    if (!api) {
        return SKULL_SERVICE_ERROR_APINAME;
    }

    // create a mock task and push it task list
    mock_task_t* task = calloc(1, sizeof(*task));
    task->req_msg = request;
    task->service = service;
    task->api = api;
    task->cb = cb;

    flist_push(env->tasks, task);

    return SKULL_SERVICE_OK;
}

// Simulate the same layout of the real sk_service_t, and beware of that if the
// real sk_service is changed, this structure has to be changed as well
typedef struct fake_service_t {
    sk_service_type_t type;

#if __WORDSIZE == 64
    int _padding;
#endif

    const char*       name; // a ref
    sk_service_opt_t  opt;

    sk_srv_data_t* data;
} fake_service_t;

struct skullut_service_t {
    fake_service_t* service;
};

skullut_service_t* skullut_service_create(const char* name, const char* config)
{
    SK_ASSERT_MSG(name, "service must contain a non-empty name\n");
    SK_ASSERT_MSG(config, "service must contain a non-empty config\n");

    skullut_service_t* ut_service = calloc(1, sizeof(*ut_service));

    // 1. create a fake service
    fake_service_t* fake_service = calloc(1, sizeof(*fake_service));
    fake_service->name = name;
    fake_service->data = sk_srv_data_create(SK_SRV_DATA_MODE_RW_PR);

    // 2. load a service
    int ret = sk_service_load((sk_service_t*)fake_service, config);
    SK_ASSERT_MSG(!ret, "service load failed\n");

    // 3. initialize service
    fake_service->opt.init((sk_service_t*)fake_service,
                           fake_service->opt.srv_data);

    ut_service->service = fake_service;
    return ut_service;
}

void skullut_service_destroy(skullut_service_t* ut_service)
{
    fake_service_t* fake_service = ut_service->service;

    fake_service->opt.release((sk_service_t*)fake_service,
                              fake_service->opt.srv_data);
    sk_service_unload((sk_service_t*)fake_service);

    sk_srv_data_destroy(fake_service->data);
    free(fake_service);
    free(ut_service);
}

void skullut_service_run(skullut_service_t* ut_service, const char* api_name,
                   const void* req_msg, skullut_service_api_validator validator,
                   void* ud)
{
    fake_service_t* fake_service = ut_service->service;
    skull_c_srvdata_t* srv_data = fake_service->opt.srv_data;
    skull_service_entry_t* entry = srv_data->entry;

    // 1. find api
    skull_service_async_api_t* api = _find_api(entry->async, api_name);
    SK_ASSERT_MSG(api, "cannot find api: %s\n", api_name);

    // 2. construct empty response
    char resp_proto_name[SKULL_SRV_PROTO_MAXLEN];
    snprintf(resp_proto_name, SKULL_SRV_PROTO_MAXLEN, "%s.%s_resp",
             fake_service->name, api_name);

    const ProtobufCMessageDescriptor* resp_desc =
        skull_srv_idl_descriptor(resp_proto_name);
    SK_ASSERT_MSG(resp_desc, "cannot find response descriptor\n");

    ProtobufCMessage* resp_msg = NULL;
    resp_msg = calloc(1, resp_desc->sizeof_message);
    protobuf_c_message_init(resp_desc, resp_msg);

    // 3. run api
    skull_service_t skull_service = {
        .service = (sk_service_t*)ut_service
    };

    api->iocall(&skull_service, req_msg, resp_msg);

    // 4. call the validator
    validator(req_msg, resp_msg, ud);

    // 5. clean up
    protobuf_c_message_free_unpacked(resp_msg, NULL);
}

void* skull_service_data (skull_service_t* service)
{
    fake_service_t* fake_service = (fake_service_t*)service->service;
    return sk_srv_data_get(fake_service->data);
}

const void* skull_service_data_const (skull_service_t* service)
{
    fake_service_t* fake_service = (fake_service_t*)service->service;
    return sk_srv_data_getconst(fake_service->data);
}

void  skull_service_data_set (skull_service_t* service, const void* data)
{
    fake_service_t* fake_service = (fake_service_t*)service->service;
    sk_srv_data_set(fake_service->data, data);
}

int skull_service_timer_create(skull_service_t* service, uint32_t delayed,
                               skull_timer_t job, void* ud,
                               skull_timer_udfree_t destroyer, int bidx)
{
    // Won't create anything
    return 0;
}

// Mock API for sk_service
const char* sk_service_name(const sk_service_t* service)
{
    return ((fake_service_t*)service)->name;
}

sk_service_opt_t* sk_service_opt(sk_service_t* service)
{
    return &((fake_service_t*)service)->opt;
}

void sk_service_setopt(sk_service_t* service, sk_service_opt_t opt)
{
    ((fake_service_t*)service)->opt = opt;
}

void sk_service_settype(sk_service_t* service, sk_service_type_t type)
{
    ((fake_service_t*)service)->type = type;
}

sk_service_type_t sk_service_type(const sk_service_t* service)
{
    return ((fake_service_t*)service)->type;
}
