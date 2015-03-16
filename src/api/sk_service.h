#ifndef SK_SERVICE_H
#define SK_SERVICE_H

#include "api/sk_config.h"
#include "api/sk_txn.h"
#include "api/sk_service_data.h"

typedef struct sk_service_t sk_service_t;

typedef enum sk_service_type_t {
    SK_C_SERICE_TYPE = 0
} sk_service_type_t;

typedef enum sk_srv_req_type_t {
    SK_SRV_REQ_TYPE_EXCLUSIVE = 0,
    SK_SRV_REQ_TYPE_RO = 1,
    SK_SRV_REQ_TYPE_WO = 2
} sk_srv_req_type_t;

typedef enum sk_srv_status_t {
    SK_SRV_STATUS_OK               = 0,
    SK_SRV_STATUS_INVALID_SRV_NAME = 1, // error: invalid service name
    SK_SRV_STATUS_BUSY             = 2, // error: cannot push new task
    SK_SRV_STATUS_TASK_FAILED      = 3,
    SK_SRV_STATUS_PENDING          = 4, // cannot pop task (pending tasks)
    SK_SRV_STATUS_IDLE             = 5  // cannot pop task (there is no task)
} sk_srv_status_t;

typedef struct sk_srv_task_t {
    sk_service_t* service;
    sk_txn_t*     txn;
    const char*   api_name;

    sk_srv_req_type_t req_type;
    sk_srv_status_t srv_status;

    const void* request;
    size_t request_sz;
} sk_srv_task_t;

typedef struct sk_service_opt_t {
    void* srv_data;

    void (*init)    (void* srv_data);
    void (*release) (void* srv_data);

    int (*io_call)  (void* srv_data, const char* api_name,
                     const void* request, size_t request_sz);
} sk_service_opt_t;

sk_service_t* sk_service_create(const char* service_name,
                                const sk_service_cfg_t* cfg);
void sk_service_destroy(sk_service_t*);

void sk_service_setopt(sk_service_t*, sk_service_opt_t opt);
void sk_service_settype(sk_service_t*, sk_service_type_t type);
void sk_service_data_construct(sk_service_t*, sk_srv_data_mode_t);

void sk_service_start(sk_service_t*);
void sk_service_stop(sk_service_t*);

const char* sk_service_name(const sk_service_t*);
sk_service_type_t sk_service_type(const sk_service_t*);
const sk_service_cfg_t* sk_service_config(const sk_service_t*);

// APIs for master
sk_srv_status_t sk_service_push_task(sk_service_t*, const sk_srv_task_t*);
int sk_service_schedule_tasks(sk_service_t*);
void sk_service_schedule_task(sk_service_t*, const sk_srv_task_t*);
void sk_service_task_complete(sk_service_t*);

// APIs for worker
sk_srv_status_t sk_service_run_iocall(sk_service_t*, const char* api_name,
                                      const void* req, size_t req_sz);

#endif

