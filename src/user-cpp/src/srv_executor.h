#ifndef SKULLCPP_SERVICE_EXECUTOR_H
#define SKULLCPP_SERVICE_EXECUTOR_H

#include <stdint.h>
#include "skull/service.h"

void skull_srv_init    (skull_service_t*, void* srv_data);

void skull_srv_release (skull_service_t*, void* srv_data);

int  skull_srv_iocall  (skull_service_t*, const char* api_name, void* srv_data);

#endif

