#ifndef SKULL_C_SERVICE_H
#define SKULL_C_SERVICE_H

#include <skull/txn.h>
#include <skull/config.h>

typedef struct skull_service_t skull_service_t;

#define SKULL_DATA_RO 0
#define SKULL_DATA_WO 1
#define SKULL_DATA_RW 2

// module callback function declartion
typedef void (*skull_module_cb) (skull_txn_t*, const void* response);

typedef struct skull_service_async_api_t {
    const char* name;
    int data_access_mode; // read-only, write-only, read-write
    int _reserved;

    const char* req_idl_name;
    const char* resp_idl_name;

    void (*iocall) (skull_service_t*, const void* request,
                    void* response);
} skull_service_async_api_t;

typedef struct skull_service_entry_t {
    int data_access_mode; // read-only, write-only, read-write
    int _reserved;

    void (*init)    (skull_service_t*, skull_config_t*);
    void (*release) (skull_service_t*);

    skull_service_async_api_t* async;
} skull_service_entry_t;

#define SKULL_SERVICE_REGISTER(entry) \
    skull_service_entry_t* skull_service_register() { \
        return entry; \
    }

int skull_service_async_call (const char* service_name, const char* api_name,
                              const void* request, skull_module_cb cb);

void  skull_service_data_set (skull_service_t*, const void* data);
void* skull_service_data (skull_service_t*);
const void* skull_service_data_const (skull_service_t*);

#endif

