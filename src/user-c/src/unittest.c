#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

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

#include "skull/metrics_utils.h"
#include "skull/ep.h"
#include "txn_utils.h"
#include "srv_types.h"

#include "skull/unittest.h"

struct skullut_module_t {
    sk_module_t*    module;
    fhash*          services; // key: name; value: mock_service

    sk_workflow_t*  workflow;
    sk_entity_t*    entity;
    sk_txn_t*       txn;
    flist*          tasks;    // hold mock_tasks

    sk_workflow_cfg_t* workflow_cfg;
    sk_module_cfg_t    module_cfg;
};

skullut_module_t* skullut_module_create(const char* module_name,
                                        const char* idl_name,
                                        const char* conf_name,
                                        const char* type,
                                        skull_module_loader_t loader)
{
    skull_module_loader_register(type, loader);

    skullut_module_t* env = calloc(1, sizeof(*env));
    env->module_cfg.name = module_name;
    env->module_cfg.type = type;

    env->module = sk_module_load(&env->module_cfg, conf_name);

    env->workflow_cfg = calloc(1, sizeof(sk_workflow_cfg_t));
    env->workflow_cfg->idl_name = idl_name;
    env->workflow_cfg->port = SK_CONFIG_NO_PORT;
    env->workflow = sk_workflow_create(env->workflow_cfg);

    env->services = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    // create a hacked entity, set 'utenv' into half_txn field, we won't use
    // half_txn in UT env, so it's safe for us to use it
    env->entity = sk_entity_create(env->workflow, SK_ENTITY_NONE);
    sk_entity_sethalftxn(env->entity, (void*)env);

    env->txn = sk_txn_create(env->workflow, env->entity);
    sk_txn_setstate(env->txn, SK_TXN_UNPACKED);
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

    sk_txn_destroy(env->txn);

    sk_entity_mark(env->entity, SK_ENTITY_INACTIVE);
    sk_entity_destroy(env->entity);

    sk_entity_mark(env->entity, SK_ENTITY_DEAD);
    sk_entity_destroy(env->entity);

    sk_workflow_destroy(env->workflow);

    // destroy the services
    fhash_str_iter srv_iter = fhash_str_iter_new(env->services);
    skullmock_svc_t* service = NULL;

    while ((service = fhash_str_next(&srv_iter))) {
        service->release(service->ud);
        free(service);
    }

    fhash_str_iter_release(&srv_iter);
    fhash_str_delete(env->services);
    flist_delete(env->tasks);

    free(env->workflow_cfg);
    skull_module_loader_unregister(env->module_cfg.type);
    free(env);
}

int skullut_module_mocksrv_add(skullut_module_t* env, skullmock_svc_t ut_svc)
{
    SK_ASSERT_MSG(ut_svc.name, "service name must not empty\n");
    skullmock_svc_t* svc = calloc(1, sizeof(ut_svc));
    memcpy(svc, &ut_svc, sizeof(ut_svc));

    fhash_str_set(env->services, ut_svc.name, svc);
    return 0;
}

int skullut_module_run(skullut_module_t* env)
{
    int ret = env->module->run(env->module->md, env->txn);

    if (flist_empty(env->tasks)) {
        return ret;
    }

    // process all service tasks
    skullmock_task_t* task = NULL;
    while ((task = flist_pop(env->tasks))) {
        // 1. prepare a response
        skullmock_svc_t* mock_svc = task->service;

        const char* api_name = task->api_name;

        // 2. call api
        mock_svc->iocall(api_name, task, mock_svc->ud);

        // 3. call module callback
        skull_txn_t skull_txn;
        skull_txn_init(&skull_txn, env->txn);

        task->cb(&skull_txn, SKULL_TXN_IO_OK, mock_svc->name, api_name,
                 task->request, task->request_sz,
                 task->response, task->response_sz, task->ud);

        // 4. clean up
        skull_txn_release(&skull_txn, env->txn);
        mock_svc->iocomplete(task, mock_svc->ud);
        free(task);
    }

    return ret;
}

void* skullut_module_data(skullut_module_t* env)
{
    sk_txn_t* txn = env->txn;
    return sk_txn_udata(txn);
}

void  skullut_module_setdata(skullut_module_t* env, const void* data)
{
    sk_txn_setudata(env->txn, data);
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

int skull_txn_peer(const skull_txn_t* skull_txn, skull_txn_peer_t* peer)
{
    if (!peer) return 1;

    peer->port   = -1;
    peer->family = -1;
    strncpy(peer->name, "Unknown", INET6_ADDRSTRLEN);
    return 0;
}

skull_txn_peer_type_t skull_txn_peertype(const skull_txn_t* skull_txn)
{
    return SKULL_TXN_PEER_T_NONE;
}

// Mock API for running mock service api in module ut
skull_txn_ioret_t
skull_txn_iocall (skull_txn_t* txn,
                  const char* service_name,
                  const char* api_name,
                  const void* request,
                  size_t request_sz,
                  skull_txn_iocb cb,
                  int bidx,
                  void* ud)
{
    if (!service_name) {
        return SKULL_TXN_IO_ERROR_SRVNAME;
    }

    sk_txn_t* sk_txn = txn->txn;
    sk_entity_t* entity = sk_txn_entity(sk_txn);
    skullut_module_t* env = (skullut_module_t*)sk_entity_halftxn(entity);

    skullmock_svc_t* service = fhash_str_get(env->services, service_name);
    if (!service) {
        return SKULL_TXN_IO_ERROR_SRVNAME;
    }

    // create a mock task and push it task list
    skullmock_task_t* task = calloc(1, sizeof(*task));
    task->service = service;
    task->cb = cb;
    task->api_name   = api_name;
    task->request    = request;
    task->request_sz = request_sz;
    task->ud         = ud;

    flist_push(env->tasks, task);
    return SKULL_TXN_IO_OK;
}

// Simulate the same layout of the real sk_service_t, and beware of that if the
// real sk_service is changed, this structure has to be changed as well
typedef struct fake_service_t {
    const char* type;
    const char* name; // a ref
    sk_service_opt_t  opt;

    sk_srv_data_t* data;
} fake_service_t;

struct skullut_service_t {
    fake_service_t* service;
};

skullut_service_t* skullut_service_create(const char* name, const char* config,
                                const char* type, skull_service_loader_t loader)
{
    SK_ASSERT_MSG(name, "service must contain a non-empty name\n");
    SK_ASSERT_MSG(config, "service must contain a non-empty config\n");
    skull_service_loader_register(type, loader);

    skullut_service_t* ut_service = calloc(1, sizeof(*ut_service));

    // 1. create a fake service
    fake_service_t* fake_service = calloc(1, sizeof(*fake_service));
    fake_service->type = type;
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
    skull_service_loader_unregister(fake_service->type);

    free(fake_service);
    free(ut_service);
}

void skullut_service_run(skullut_service_t* ut_service, const char* api_name,
                   const void* req, size_t req_sz,
                   skullut_service_api_validator validator,
                   void* ud)
{
    fake_service_t* fake_service = ut_service->service;
    skull_service_opt_t* opt = fake_service->opt.srv_data;

    // run api
    sk_txn_taskdata_t taskdata;
    memset(&taskdata, 0, sizeof(taskdata));
    taskdata.request = req;
    taskdata.request_sz = req_sz;

    skull_service_t skull_service = {
        .service = (sk_service_t*)fake_service,
        .task    = &taskdata,
        .freezed = 0
    };

    opt->iocall(&skull_service, api_name, opt->ud);

    // 4. call the validator
    validator(taskdata.request, taskdata.request_sz,
              taskdata.response, taskdata.response_sz, ud);
}

// Service Mock APIs
void* skull_service_data (skull_service_t* service)
{
    fake_service_t* fake_service = (fake_service_t*)service->service;
    return sk_srv_data_get(fake_service->data);
}

const void* skull_service_data_const (const skull_service_t* service)
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
                               skull_job_t job, void* ud,
                               skull_job_udfree_t destroyer, int bidx)
{
    // Won't create anything
    return 0;
}

// TODO: the apidata_set, apidata, apiname apis should be linked directly instead
//  of copy the code to here
int skull_service_apidata_set(skull_service_t* svc, int type,
                              const void* data, size_t sz)
{
    sk_txn_taskdata_t* taskdata = svc->task;
    if (!taskdata) {
        return 1;
    }

    if (type == SKULL_API_REQ) {
        taskdata->request    = data;
        taskdata->request_sz = sz;
    } else {
        taskdata->response   = (void*)data;
        taskdata->response_sz = sz;
    }

    return 0;
}

void* skull_service_apidata(skull_service_t* svc, int type, size_t* sz)
{
    sk_txn_taskdata_t* taskdata = svc->task;
    if (!taskdata) {
        return NULL;
    }

    if (type == SKULL_API_REQ) {
        if (sz) *sz = taskdata->request_sz;
        return (void*)taskdata->request;
    } else {
        if (sz) *sz = taskdata->response_sz;
        return taskdata->response;
    }
}

const char* skull_service_apiname(const skull_service_t* svc)
{
    sk_txn_taskdata_t* taskdata = svc->task;
    if (!taskdata) {
        return NULL;
    }

    return taskdata->api_name;
}

// Mock API for sk_service
const char* sk_service_name(const sk_service_t* service)
{
    return ((fake_service_t*)service)->name;
}

const char* skull_service_name(const skull_service_t* service)
{
    fake_service_t* fake_service = (fake_service_t*)service->service;
    return fake_service->name;
}

sk_service_opt_t* sk_service_opt(sk_service_t* service)
{
    return &((fake_service_t*)service)->opt;
}

void sk_service_setopt(sk_service_t* service, sk_service_opt_t opt)
{
    ((fake_service_t*)service)->opt = opt;
}

const char* sk_service_type(const sk_service_t* service)
{
    return ((fake_service_t*)service)->type;
}

void sk_service_api_register(sk_service_t* service, const char* api_name,
                             sk_service_api_type_t type)
{
    return;
}

// No one use it, just return NULL
const sk_service_api_t* sk_service_api(const sk_service_t* svc, const char* api)
{
    return NULL;
}

skull_job_ret_t
skull_service_job_create(skull_service_t*   svc,
                         uint32_t           delayed,
                         skull_job_rw_t     type,
                         skull_job_t        job,
                         void*              ud,
                         skull_job_udfree_t udfree)
{
    return SKULL_JOB_OK;
}

skull_job_ret_t
skull_service_job_create_np(skull_service_t*   svc,
                            uint32_t           delayed,
                            skull_job_rw_t     type,
                            skull_job_np_t     job,
                            void*              ud,
                            skull_job_udfree_t udfree,
                            int                bio_idx)
{
    return SKULL_JOB_OK;
}

// Mock API for skull_ep_xxx_send. Return ok directly, since we don't need to mock
// network, only mock the api response is enough
skull_ep_status_t
skull_ep_send(const skull_service_t* svc, const skull_ep_handler_t handler,
            const void* data, size_t count, skull_ep_cb_t cb, void* ud)
{
    return SKULL_EP_OK;
}

skull_ep_status_t
skull_ep_send_np(const skull_service_t* svc, const skull_ep_handler_t handler,
              const void* data, size_t count, skull_ep_np_cb_t cb, void* ud)
{
    return SKULL_EP_OK;
}
