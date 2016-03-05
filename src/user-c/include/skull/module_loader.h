#ifndef SKULL_MODULE_LOADER_H
#define SKULL_MODULE_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "skull/module.h"

typedef struct skull_module_loader_t {
    const char* (*name)      (const char* short_name,
                              char* fullname, size_t sz);
    const char* (*conf_name) (const char* short_name,
                              char* confname, size_t confname_sz);

    skull_module_t* (*open)        (const char* filename);
    int             (*load_config) (skull_module_t*, const char* filename);
    int             (*close)       (skull_module_t* module);
} skull_module_loader_t;

void skull_module_register_loader(const char* type,
                                  skull_module_loader_t loader);

#ifdef __cplusplus
}
#endif

#endif

