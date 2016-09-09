#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include <Python.h>
#pragma GCC diagnostic pop

#include "skull/api.h"
#include "skullcpp/module_loader.h"
#include "module_executor.h"
#include "module_loader.h"

#define SKULL_MODULE_CONF_PREFIX_NAME "skull-modules-"

#define PYTHON_API_LOADER_NAME        "skullpy.module_loader"
#define PYTHON_API_LOADER_FUNCNAME    "module_load"
#define PYTHON_API_EXECUTOR_FUNCNAME  "module_execute"

static
const char* _module_name(const char* short_name, char* fullname, size_t sz)
{
    memset(fullname, 0, sz);

    // The full name is the same as short_name for Python
    snprintf(fullname, sz, "%s", short_name);
    return fullname;
}

static
const char* _module_conf_name(const char* short_name, char* confname, size_t sz)
{
    memset(confname, 0, sz);

    // The full name format: etc/libskull-modules-%s.yaml
    snprintf(confname, sz, "etc/" SKULL_MODULE_CONF_PREFIX_NAME "%s.yaml",
             short_name);
    return confname;
}

static
skull_module_t* _module_open(const char* module_name)
{
    // 1. Import python module_loader
    PyObject* pyLoaderModuleName = PyString_FromString(PYTHON_API_LOADER_NAME);
    if (!pyLoaderModuleName) {
        fprintf(stderr, "Cannot construct python module name: %s", PYTHON_API_LOADER_NAME);
        return NULL;
    }

    PyObject* pyLoaderModule = PyImport_Import(pyLoaderModuleName);
    Py_DECREF(pyLoaderModuleName);

    if (!pyLoaderModule) {
        fprintf(stderr, "Cannot import python api loader module: %s", PYTHON_API_LOADER_NAME);
        return NULL;
    }

    PyObject* pyLoaderFunc = PyObject_GetAttrString(pyLoaderModule, PYTHON_API_LOADER_FUNCNAME);
    if (!pyLoaderFunc || !PyCallable_Check(pyLoaderFunc)) {
        fprintf(stderr, "Cannot import python module loader function: %s", PYTHON_API_LOADER_FUNCNAME);
        PyErr_Print();
        return NULL;
    }

    PyObject* pyExecutorFunc = PyObject_GetAttrString(pyLoaderModule, PYTHON_API_EXECUTOR_FUNCNAME);
    if (!pyExecutorFunc || !PyCallable_Check(pyExecutorFunc)) {
        fprintf(stderr, "Cannot import python module executor function: %s", PYTHON_API_EXECUTOR_FUNCNAME);
        PyErr_Print();
        return NULL;
    }

    // 2. create module and its private data
    skull_module_t* module = (skull_module_t*)calloc(1, sizeof(*module));
    skullpy::module_data_t* md = (skullpy::module_data_t*)calloc(1, sizeof(*md));
    md->pyLoaderModule = pyLoaderModule;
    md->pyLoaderFunc   = pyLoaderFunc;
    md->pyExecutorFunc = pyExecutorFunc;
    printf("loaderModule Ref: %lu, loaderFunc Ref: %lu, pyExecutorFunc Ref: %lu\n",
           Py_REFCNT(md->pyLoaderModule), Py_REFCNT(md->pyLoaderFunc), Py_REFCNT(md->pyExecutorFunc));

    // 3. load module entry
    PyObject* pyArgs = PyTuple_New(1);
    PyObject* pyModuleName = PyString_FromString(module_name);
    PyTuple_SetItem(pyArgs, 0, pyModuleName);

    PyObject* ret = PyObject_CallObject(md->pyLoaderFunc, pyArgs);
    Py_DECREF(pyArgs);

    if (!ret) {
        fprintf(stderr, "Error: Cannot load user module: %s", module_name);
        PyErr_Print();
        return NULL;
    }

    // 4. Assemble execution point
    module->init    = skull_module_init;
    module->run     = skull_module_run;
    module->unpack  = skull_module_unpack;
    module->pack    = skull_module_pack;
    module->release = skull_module_release;
    module->ud = md;
    md->name = strdup(module_name);

    return module;
}

static
int _module_close(skull_module_t* module)
{
    skullpy::module_data_t* md = (skullpy::module_data_t*)module->ud;
    skull_config_destroy(md->config);
    Py_XDECREF(md->pyLoaderModule);
    Py_XDECREF(md->pyLoaderFunc);
    Py_XDECREF(md->pyExecutorFunc);

    free((void*)md->name);
    free(md);
    free(module);
    return 0;
}

static
int _module_load_config(skull_module_t* module, const char* filename)
{
    if (!filename) {
        fprintf(stderr, "error: module config name is NULL\n");
        return 1;
    }

    skullpy::module_data_t* md = (skullpy::module_data_t*)module->ud;
    md->config = skull_config_create(filename);
    return 0;
}

skull_module_loader_t module_getloader() {
    skull_module_loader_t loader;
    loader.name        = _module_name;
    loader.conf_name   = _module_conf_name;
    loader.load_config = _module_load_config;
    loader.open        = _module_open;
    loader.close       = _module_close;

    return loader;
}

// Module Loader Register
void skullpy_module_loader_register()
{
    skull_module_loader_t loader = module_getloader();
    skull_module_loader_register("py", loader);
}

void skullpy_module_loader_unregister()
{
    skull_module_loader_unregister("py");
}
