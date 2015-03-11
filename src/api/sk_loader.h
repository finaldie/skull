#ifndef SK_LOADER_H
#define SK_LOADER_H

#include "api/sk_module.h"

typedef struct sk_module_loader_t {
    // module loader type
    sk_mtype_t type;

#if __WORDSIZE == 64
    int         padding;
#endif

    const char* (*name)      (const char* short_name,
                              char* fullname, size_t sz);
    const char* (*conf_name) (const char* short_name,
                              char* confname, size_t confname_sz);

    sk_module_t* (*open)        (const char* filename);
    int          (*load_config) (sk_module_t*, const char* filename);
    int          (*close)       (sk_module_t* module);
} sk_module_loader_t;

/**
 * load a module
 *
 * @note short_name is a short name, for example "test", so it may a C
 * or a cpp/lua/python... module
 *
 * @param[in] short_name    short name of a module
 * @param[in] config_name   config name of a module
 * @return a module object if success or NULL if failure
 */
sk_module_t* sk_module_load(const char* short_name,
                            const char* config_name);

void sk_module_unload(sk_module_t* module);

// =============================================================================

#include "api/sk_service.h"

typedef struct sk_service_loader_t {
    // service loader type
    sk_service_type_t type;

#if __WORDSIZE == 64
    int         padding;
#endif

    const char* (*name)      (const char* short_name,
                              char* fullname, size_t sz);
    const char* (*conf_name) (const char* short_name,
                              char* confname, size_t confname_sz);

    sk_service_t* (*open)  (const char* service_name, const char* filename);
    int           (*close) (sk_service_t* service);

    int           (*load_config) (sk_service_t*, const char* filename);
} sk_service_loader_t;

sk_service_t* sk_service_load(const char* service_name,
                              const char* config_name);

void sk_service_unload(sk_service_t* service);

#endif

