#ifndef SKULL_SERVICE_EXECUTOR_H
#define SKULL_SERVICE_EXECUTOR_H

#include <stdint.h>
#include "api/sk_txn.h"
#include "api/sk_service.h"

void skull_srv_init    (sk_service_t*, void* srv_data);

void skull_srv_release (sk_service_t*, void* srv_data);

int  skull_srv_iocall  (sk_service_t*, sk_txn_t*, void* srv_data,
                        uint64_t task_id, const char* api_name,
                        sk_srv_io_status_t ustatus);

#endif

