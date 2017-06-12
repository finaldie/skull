#ifndef SKULL_SERVICE_LOADER_H
#define SKULL_SERVICE_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <skull/service.h>

typedef struct skull_service_opt_t {
    void* ud;

    int  (*init)    (skull_service_t*, void* ud);
    void (*release) (skull_service_t*, void* ud);

    int  (*iocall)  (skull_service_t*, const char* api_name, void* ud);
    void (*iocomplete) (skull_service_t*, const char* api_name, void* ud);
} skull_service_opt_t;

typedef struct skull_service_loader_t {
    const char* (*name)       (const char* short_name,
                               char* fullname, size_t sz);
    const char* (*conf_name)  (const char* short_name,
                               char* confname, size_t confname_sz);

    int (*open)  (const char* filename, skull_service_opt_t* /*out*/);
    int (*close) (skull_service_opt_t*);

    int (*load_config) (skull_service_opt_t*, const char* user_cfg_filename);
} skull_service_loader_t;

void skull_service_loader_register(const char* type,
                                   skull_service_loader_t loader);
void skull_service_loader_unregister(const char* type);

typedef enum skull_service_api_type_t {
    SKULL_SVC_API_READ  = 0,
    SKULL_SVC_API_WRITE = 1
} skull_service_api_type_t;

void skull_service_api_register(skull_service_t*, const char* api_name,
                                skull_service_api_type_t type);

#ifdef __cplusplus
}
#endif

#endif

