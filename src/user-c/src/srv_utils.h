#ifndef SKULL_SVC_UTILS_H
#define SKULL_SVC_UTILS_H

#include "skull/service.h"

skull_service_async_api_t*
skull_svc_find_api(skull_service_async_api_t** apis, const char* api_name);

#endif

