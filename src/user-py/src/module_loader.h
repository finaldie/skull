#ifndef SKULLPY_MOD_LOADER_H
#define SKULLPY_MOD_LOADER_H

#include <Python.h>
#include <skull/api.h>

namespace skullpy {

typedef struct module_data_t {
    const skull_config_t* config;
    const char* name;

    PyObject* pyLoaderModule;
    PyObject* pyLoaderFunc;   // Module Loader
    PyObject* pyExecutorFunc; // Module Executor
} module_data_t;

} // End of namespace

void skullpy_module_loader_register();
void skullpy_module_loader_unregister();
skull_module_loader_t module_getloader();

#endif

