#include <stdlib.h>

#include "api/sk_utils.h"
#include "srv_loader.h"
#include "srv_executor.h"

struct skull_service_t {
    sk_service_t* service;
};

void skull_srv_init (sk_service_t* srv, void* data)
{
    skull_c_srvdata* srv_data = data;
    skull_service_entry_t* entry = srv_data->entry;

    skull_service_t skull_service = {
        .service = srv
    };

    entry->init(&skull_service, srv_data->config);
}

void skull_srv_release (sk_service_t* srv, void* data)
{
    skull_c_srvdata* srv_data = data;
    skull_service_entry_t* entry = srv_data->entry;

    skull_service_t skull_service = {
        .service = srv
    };

    entry->release(&skull_service);
}

/**
 * Invoke the real service api function
 *
 *
 */
int  skull_srv_iocall  (sk_service_t* srv, void* srv_data, const char* api_name,
                        sk_srv_io_status_t ustatus,
                        const void* request, size_t request_sz)
{
    // find the api func
    
    return 0;
}
