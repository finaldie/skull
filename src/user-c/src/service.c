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

const void* skull_service_data_const (skull_service_t* service)
{
    if (service->freezed) {
        return NULL;
    }

    sk_service_t* sk_srv = service->service;
    SK_ASSERT(sk_srv);
    return sk_service_data_const(sk_srv);
}

typedef struct timer_data_t {
    skull_service_t    svc;
    skull_job_t        job;
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

    if (valid) {
        skull_service_t service = jobdata->svc;
        service.freezed = 0;

        jobdata->job(&service, jobdata->ud);
    } else {
        sk_print("skull serivce: timer is not valid, ignore it\n");
    }
}

int skull_service_job_create(skull_service_t* service, uint32_t delayed,
                               skull_job_t job, void* ud,
                               skull_job_udfree_t udfree, int bidx)
{
    sk_service_t* sk_svc = service->service;

    timer_data_t* jobdata = calloc(1, sizeof(*jobdata));
    jobdata->svc       = *service;
    jobdata->job       = job;
    jobdata->destroyer = udfree;
    jobdata->ud        = ud;

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

const char* skull_service_name(skull_service_t* service)
{
    sk_service_t* svc = service->service;
    return sk_service_name(svc);
}
