#ifndef SKULLCPP_MOD_LOADER_H
#define SKULLCPP_MOD_LOADER_H

#include <skull/api.h>
#include <skullcpp/module_loader.h>

namespace skullcpp {

typedef struct module_data_t {
    void*                 handler;
    const skull_config_t* config;

    ModuleEntry*          entry;
    ModuleEntry*          (*reg)();
} module_data_t;

} // End of namespace

void skullcpp_module_loader_register();
void skullcpp_module_loader_unregister();
skull_module_loader_t module_getloader();

#endif

