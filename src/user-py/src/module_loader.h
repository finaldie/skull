#ifndef SKULLPY_MOD_LOADER_H
#define SKULLPY_MOD_LOADER_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include <Python.h>
#pragma GCC diagnostic pop

#include "skull/api.h"

namespace skullpy {

typedef struct module_data_t {
    const skull_config_t* config;
    const char* name;

    PyObject* pyLoaderModule;
    PyObject* pyLoadingFunc;      // Module Load
    PyObject* pyLoadingConfFunc;  // Module Load Config
    PyObject* pyExecutorFunc;     // Module Executor
} module_data_t;

void module_loader_register();
void module_loader_unregister();

} // End of namespace

#endif

