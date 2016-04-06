#ifndef SK_LOADER_H
#define SK_LOADER_H

#include "flibs/fhash.h"
#include "api/sk_module.h"
#include "api/sk_config.h"

typedef struct sk_module_loader_t {
    const char* (*name)      (const char* short_name,
                              char* fullname, size_t sz, void* ud);
    const char* (*conf_name) (const char* short_name,
                              char* confname, size_t confname_sz, void* ud);

    sk_module_t* (*open)        (const char* filename, void* ud);
    int          (*load_config) (sk_module_t*, const char* filename, void* ud);
    int          (*close)       (sk_module_t* module, void* ud);

    void* ud;
} sk_module_loader_t;

/**
 * load a module
 *
 * @note short_name is a short name, for example "test", so it may a C
 * or a cpp/lua/python... module
 *
 * @param[in] cfg           static config of a module, including type and name
 * @param[in] config_name   config name of a module
 * @return a module object if success or NULL if failure
 */
sk_module_t* sk_module_load(const sk_module_cfg_t* cfg,
                            const char* config_name);

/**
 * unload a module and release the core layer data
 */
void sk_module_unload(sk_module_t* module);

void sk_module_loader_register(const char* type, sk_module_loader_t loader);
sk_module_loader_t* sk_module_loader_unregister(const char* type);

// =============================================================================

#include "api/sk_service.h"

typedef struct sk_service_loader_t {
    const char* (*name)       (const char* short_name,
                               char* fullname, size_t sz, void* ud);
    const char* (*conf_name)  (const char* short_name,
                               char* confname, size_t confname_sz, void* ud);

    int (*open)  (const char* filename, sk_service_opt_t* /*out*/, void* ud);
    int (*close) (sk_service_t* service, void* ud);

    int (*load_config) (sk_service_t*, const char* user_cfg_filename, void* ud);

    void* ud;
} sk_service_loader_t;

/**
 * load a service
 *
 * @param srv              a pointer of sk_service_t
 * @param user_config_name
 *
 * @return 0 if success, 1 if failure
 */
int sk_service_load(sk_service_t* srv, const char* user_config_name);

/**
 * unload a service and release the core layer data
 */
void sk_service_unload(sk_service_t* service);

void sk_service_loader_register(const char* type, sk_service_loader_t loader);
sk_service_loader_t* sk_service_loader_unregister(const char* type);

/******************************************************************************/
int sk_userlib_load(const char* libname);
void sk_userlib_unload();

#endif

