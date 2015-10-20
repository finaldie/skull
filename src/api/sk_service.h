#ifndef SK_SERVICE_H
#define SK_SERVICE_H

#include "api/sk_config.h"
#include "api/sk_txn.h"
#include "api/sk_queue.h"
#include "api/sk_service_data.h"

typedef struct sk_service_t sk_service_t;

typedef enum sk_service_type_t {
    SK_C_SERVICE_TYPE = 0
} sk_service_type_t;

// service status
typedef enum sk_srv_status_t {
    SK_SRV_STATUS_OK               = 0,

    // for user
    SK_SRV_STATUS_BUSY             = 1, // error: cannot push new task

    // internal
    SK_SRV_STATUS_TASK_FAILED      = 2,
    SK_SRV_STATUS_PENDING          = 3, // cannot pop task (pending tasks)
    SK_SRV_STATUS_IDLE             = 4, // cannot pop task (there is no task)
    SK_SRV_STATUS_NOT_PUSHED       = 5
} sk_srv_status_t;

// service status for user layer
typedef enum sk_srv_io_status_t {
    SK_SRV_IO_STATUS_OK = 0,
    SK_SRV_IO_STATUS_INVALID_SRV_NAME = 1,
    SK_SRV_IO_STATUS_BUSY = 2,
    SK_SRV_IO_STATUS_MAX // DO NOT USE IT
} sk_srv_io_status_t;

typedef struct sk_srv_task_t {
    sk_queue_elem_base_t base;  // This must be placed at the front
    sk_srv_io_status_t   io_status;

    sk_service_t* service;
    sk_txn_t*     txn;
    const char*   api_name;
    uint64_t      task_id;
} sk_srv_task_t;

typedef struct sk_service_opt_t {
    void* srv_data;

    void (*init)    (sk_service_t*, void* srv_data);
    void (*release) (sk_service_t*, void* srv_data);

    int  (*io_call) (sk_service_t*, sk_txn_t*, void* srv_data, uint64_t task_id,
                     const char* api_name, sk_srv_io_status_t ustatus);
} sk_service_opt_t;

typedef struct sk_service_api_t {
    sk_srv_api_cfg_t* cfg;
} sk_service_api_t;

sk_service_t* sk_service_create(const char* service_name,
                                const sk_service_cfg_t* cfg);
void sk_service_destroy(sk_service_t*);

void sk_service_setopt(sk_service_t*, const sk_service_opt_t opt);
sk_service_opt_t* sk_service_opt(sk_service_t*);
void sk_service_settype(sk_service_t*, sk_service_type_t type);

const sk_service_api_t* sk_service_api(sk_service_t*, const char* api_name);

void sk_service_start(sk_service_t*);
void sk_service_stop(sk_service_t*);

const char* sk_service_name(const sk_service_t*);
sk_service_type_t sk_service_type(const sk_service_t*);
const sk_service_cfg_t* sk_service_config(const sk_service_t*);

// APIs for master
sk_srv_status_t sk_service_push_task(sk_service_t*, const sk_srv_task_t*);
size_t sk_service_schedule_tasks(sk_service_t*);
void sk_service_schedule_task(sk_service_t*, const sk_srv_task_t*);
void sk_service_task_setcomplete(sk_service_t*);

// APIs for worker
sk_srv_status_t sk_service_run_iocall(sk_service_t*, sk_txn_t* txn,
                                      uint64_t task_id,
                                      const char* api_name,
                                      sk_srv_io_status_t io_status);

// APIs for user
//  Data APIs (Experimental)
void* sk_service_data(sk_service_t*);
const void* sk_service_data_const(sk_service_t*);
void sk_service_data_set(sk_service_t*, const void* data);

//  Invoke Service IO call
int sk_service_iocall(sk_service_t*, sk_txn_t* txn, const char* api_name,
                      const void* req, size_t req_sz,
                      sk_txn_module_cb cb, void* ud);

#endif

