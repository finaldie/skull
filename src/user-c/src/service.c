#include <stdlib.h>
#include <string.h>

#include "api/sk_core.h"
#include "api/sk_env.h"
#include "api/sk_pto.h"
#include "api/sk_utils.h"
#include "api/sk_object.h"

#include "txn_utils.h"
#include "srv_types.h"
#include "skull/txn.h"
#include "skull/service.h"

skull_service_ret_t
skull_service_async_call (skull_txn_t* txn, const char* service_name,
                          const char* api_name, const void* request,
                          size_t request_sz, skull_svc_api_cb cb, int bidx)
{
    sk_core_t* core = SK_ENV_CORE;
    sk_service_t* service = sk_core_service(core, service_name);
    if (!service) {
        return SKULL_SERVICE_ERROR_SRVNAME;
    }

    // find the exact async api
    const sk_service_api_t* api = sk_service_api(service, api_name);
    if (!api) {
        return SKULL_SERVICE_ERROR_APINAME;
    }

    // serialize the request data
    int ioret = sk_service_iocall(service, txn->txn, api_name,
                      request, request_sz,
                      (sk_txn_task_cb)cb, txn, bidx);

    if (ioret) {
        return SKULL_SERVICE_ERROR_BIO;
    }

    return SKULL_SERVICE_OK;
}

void  skull_service_data_set (skull_service_t* service, const void* data)
{
    if (service->freezed) {
        return;
    }

    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);

    sk_service_data_set(sk_srv, data);
}

void* skull_service_data (skull_service_t* service)
{
    if (service->freezed) {
        return NULL;
    }

    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);
    return sk_service_data(sk_srv);
}

const void* skull_service_data_const (const skull_service_t* service)
{
    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);
    return sk_service_data_const(sk_srv);
}

typedef enum svc_job_type_t {
    PENDING   = 0,
    NOPENDING = 1
} svc_job_type_t;

typedef struct timer_data_t {
    skull_service_t    svc;
    svc_job_type_t     type;

#if __WORDSIZE == 64
    int __padding;
#endif

    union {
        skull_job_t    job;
        skull_job_np_t job_np;
    } cb_;

    skull_job_udfree_t destroyer;
    void* ud;
} timer_data_t;

static
void _timer_data_destroy(sk_ud_t ud)
{
    timer_data_t* data = ud.ud;

    if (data->destroyer) {
        data->destroyer(data->ud);
    }

    free(data);
}

static
void _timer_cb (sk_service_t* sk_svc, sk_obj_t* ud, int valid)
{
    sk_print("skull service: timer cb, valid: %d\n", valid);
    timer_data_t* jobdata = sk_obj_get(ud).ud;
    SK_ASSERT_MSG(valid, "Currently we won't cancel any user timer, it must be something wrong");

    if (!valid) {
        sk_print("Error: skull serivce: timer is not valid, ignore it\n");
        return;
    }

    skull_service_t service = jobdata->svc;
    //service.txn     = NULL;
    //service.task    = NULL;
    service.freezed = 0;

    if (jobdata->type == PENDING) {
        sk_txn_taskdata_t* task_data = service.task;
        SK_ASSERT(task_data);
        SK_ASSERT(service.txn);

        jobdata->cb_.job(&service, jobdata->ud,
                         task_data->request, task_data->request_sz,
                         task_data->response, task_data->response_sz);

        // Reduce pending tasks counts
        task_data->pendings--;
        sk_print("service task pending cnt: %u\n", task_data->pendings);

        // Try to call api callback
        sk_service_api_complete(sk_svc, service.txn,
                                task_data, task_data->api_name);
    } else {
        jobdata->cb_.job_np(&service, jobdata->ud);
    }
}

int skull_service_job_create(skull_service_t* service, uint32_t delayed,
                             skull_job_t job, void* ud,
                             skull_job_udfree_t udfree)
{
    if (!service->task || !service->txn) {
        sk_print("Error: skull service: a pending service job only can be created in a service api\n");

        if (udfree) {
            udfree(ud);
        }
        return 1;
    }

    sk_service_t* sk_svc = service->service;

    timer_data_t* jobdata = calloc(1, sizeof(*jobdata));
    jobdata->svc       = *service;
    jobdata->type      = PENDING;
    jobdata->cb_.job   = job;
    jobdata->destroyer = udfree;
    jobdata->ud        = ud;

    sk_ud_t      cb_data = {.ud = jobdata};
    sk_obj_opt_t opt     = {.preset = NULL, .destroy = _timer_data_destroy};
    sk_obj_t*    param_obj = sk_obj_create(opt, cb_data);

    // Fix the bio index to 0 due to we can not schedule it to other thread,
    //  or it would leading a contention issue of *task_data* memory
    int ret = sk_service_job_create(sk_svc, delayed, _timer_cb, param_obj, 0);
    if (ret) {
        sk_obj_destroy(param_obj);
    } else {
        service->task->pendings++;
        sk_print("service task pending cnt: %u\n", service->task->pendings);
    }

    return ret;
}

int skull_service_job_create_np(skull_service_t* service, uint32_t delayed,
                                skull_job_np_t job, void* ud,
                                skull_job_udfree_t udfree, int bidx)
{
    sk_service_t* sk_svc = service->service;

    timer_data_t* jobdata = calloc(1, sizeof(*jobdata));
    jobdata->svc        = *service;
    jobdata->type       = NOPENDING;
    jobdata->cb_.job_np = job;
    jobdata->destroyer  = udfree;
    jobdata->ud         = ud;

    sk_ud_t      cb_data = {.ud = jobdata};
    sk_obj_opt_t opt     = {.preset = NULL, .destroy = _timer_data_destroy};
    sk_obj_t*    param_obj = sk_obj_create(opt, cb_data);

    int ret = sk_service_job_create(sk_svc, delayed, _timer_cb, param_obj, bidx);
    if (ret) {
        sk_obj_destroy(param_obj);
    }

    return ret;
}

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
        taskdata->response    = (void*)data;
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

const char* skull_service_name(const skull_service_t* service)
{
    sk_service_t* svc = service->service;
    return sk_service_name(svc);
}
