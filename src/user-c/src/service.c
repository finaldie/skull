#include <stdlib.h>

#include <skull/service.h>

int skull_service_async_call (const char* service_name, const char* api_name,
                              const void* request, skull_module_cb cb)
{
    return 0;
}

void  skull_service_data_set (skull_service_t* service, const void* data)
{

}

void* skull_service_data (skull_service_t* service)
{

    return NULL;
}

const void* skull_service_data_const (skull_service_t* service)
{
    return NULL;
}
