#include <stdlib.h>
#include <string.h>

#include "srv_utils.h"

skull_service_async_api_t*
skull_svc_find_api(skull_service_async_api_t** apis, const char* api_name)
{
    if (!apis || !api_name) {
        return NULL;
    }

    for (int i = 0; apis[i] != NULL; i++) {
        skull_service_async_api_t* api = apis[i];

        if (0 == strcmp(api->name, api_name)) {
            return api;
        }
    }

    return NULL;
}
