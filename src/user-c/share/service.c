#include <stdlib.h>
#include <stdio.h>

#include <skull/api.h>
#include "skull_metrics.h"
#include "config.h"
#include "skull_srv_api_proto.h"

static
void skull_service_init(skull_service_t* service, skull_config_t* config)
{
    printf("skull service init\n");

    // Convert skull_config to skull_static_config
    skull_static_config_convert(config);

    // Register service idls
    skull_srv_idl_register(skull_srv_api_desc_tbl);
}

static
void skull_service_release(skull_service_t* service)
{
    printf("skull service release\n");
}

// ====================== Service APIs Calls ===================================
static
void skull_service_getdata(skull_service_t* service, const void* request,
                           void* response)
{
    printf("skull service api: getdata\n");
}

static
void skull_service_setdata(skull_service_t* service, const void* request,
                           void* response)
{
    printf("skull service api: setdata\n");
}

// ====================== Service APIs Calls End ===============================

static
skull_service_async_api_t test_get = {
    .name             = "get",
    .data_access_mode = SKULL_DATA_RO,
    .iocall           = skull_service_getdata
};

static
skull_service_async_api_t test_set = {
    .name             = "set",
    .data_access_mode = SKULL_DATA_WO,
    .iocall           = skull_service_setdata
};

static
skull_service_async_api_t* api_tbl[] = {
    &test_get,
    &test_set,
    NULL
};

static
skull_service_entry_t service_entry = {
    .init = skull_service_init,
    .release = skull_service_release,
    .async = api_tbl
};

SKULL_SERVICE_REGISTER(&service_entry)
