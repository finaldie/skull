#ifndef SKULL_C_SERVICE_H
#define SKULL_C_SERVICE_H

#include <stdint.h>
#include <skull/txn.h>
#include <skull/config.h>

typedef struct _skull_service_t skull_service_t;

// ===================== APIs and Data Structures for Service ==================
#define SKULL_SRV_PROTO_MAXLEN 64

// Notes:
//  - request proto name is 'service.name_req'
//  - response proto name is 'service.name_resp'
typedef struct skull_service_async_api_t {
    const char* name;
    void (*iocall) (skull_service_t*, const void* request, void* response);
} skull_service_async_api_t;

typedef struct skull_service_entry_t {
    void (*init)    (skull_service_t*, skull_config_t*);
    void (*release) (skull_service_t*);

    skull_service_async_api_t** async;
} skull_service_entry_t;

#define SKULL_SERVICE_REGISTER(entry) \
    skull_service_entry_t* skull_service_register() { \
        return entry; \
    }

void  skull_service_data_set (skull_service_t*, const void* data);
void* skull_service_data (skull_service_t*);
const void* skull_service_data_const (skull_service_t*);

// ===================== APIs and Data Structures for Module ===================

// module callback function declartion
typedef int (*skull_svc_api_cb) (skull_txn_t*, const void* request,
                                 const void* response);

typedef enum skull_service_ret_t {
    SKULL_SERVICE_OK            = 0,
    SKULL_SERVICE_ERROR_SRVNAME = 1,
    SKULL_SERVICE_ERROR_APINAME = 2,
    SKULL_SERVICE_ERROR_BIO     = 3
} skull_service_ret_t;

/**
 * Invoke a service async call
 *
 * @param serivce_name
 * @param api_name
 * @param request       request protobuf message
 * @param cb            module callback function
 * @param bio_idx       background io index
 *                      - (-1)  : random pick up a background io to run
 *                      - (0)   : do not use background io
 *                      - (> 0) : run on the index of background io
 *
 * @return - SKULL_SERVICE_OK
 *         - SKULL_SERVICE_ERROR_SRVNAME
 *         - SKULL_SERVICE_ERROR_APINAME
 */
skull_service_ret_t
skull_service_async_call (skull_txn_t*,
                          const char* service_name,
                          const char* api_name,
                          const void* request,
                          skull_svc_api_cb cb,
                          int bio_idx);

typedef void (*skull_job_t) (skull_service_t*, void* ud);
typedef void (*skull_job_udfree_t) (void* ud);

/**
 * Create a service job
 *
 * @param delayed  unit milliseconds
 * @param timer    job callback function
 * @param ud       user data
 * @param udfree   the function is used for releasing user data after timer be
 *                  finished
 * @param bio_idx  background io idx
 *                  - (-1)  : random pick up one
 *                  - (0)   : do not use bio
 *                  - (> 0) : use the idx of bio
 *
 * @return 0 on success, 1 on failure
 */
int skull_service_job_create(skull_service_t*   svc,
                             uint32_t           delayed,
                             skull_job_t        timer,
                             void*              ud,
                             skull_job_udfree_t udfree,
                             int                bio_idx);

#endif

