#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include <Python.h>
#pragma GCC diagnostic pop

#include <stdlib.h>
#include <string.h>

#include "py3_compat.h"
#include "module_executor.h"
#include "module_loader.h"

#define SKULL_MODULE_CONF_PREFIX_NAME    "skull-modules-"

#define PYTHON_API_LOADER_NAME           "skull.module_loader"
#define PYTHON_API_LOADING_FUNCNAME      "module_load"
#define PYTHON_API_LOADING_CONF_FUNCNAME "module_load_config"
#define PYTHON_API_EXECUTOR_FUNCNAME     "module_execute"

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
    PyGILState_STATE state = PyGILState_Ensure();

    // 1. Import python module_loader
    PyObject* pyLoaderModuleName = PyString_FromString(PYTHON_API_LOADER_NAME);
    if (!pyLoaderModuleName) {
        fprintf(stderr, "Cannot construct python module name: %s",
                PYTHON_API_LOADER_NAME);
        return NULL;
    }

    PyObject* pyLoaderModule = PyImport_Import(pyLoaderModuleName);
    Py_DECREF(pyLoaderModuleName);

    if (!pyLoaderModule) {
        fprintf(stderr, "Cannot import python api loader module: %s",
                PYTHON_API_LOADER_NAME);
        PyErr_Print();
        return NULL;
    }

    // 2. Loading basic enties from loader module
    // 2.1 Load 'module_load' from loader
    PyObject* pyLoadingFunc =
        PyObject_GetAttrString(pyLoaderModule, PYTHON_API_LOADING_FUNCNAME);

    if (!pyLoadingFunc || !PyCallable_Check(pyLoadingFunc)) {
        fprintf(stderr, "Cannot import python module loader function: %s",
                PYTHON_API_LOADING_FUNCNAME);
        PyErr_Print();
        return NULL;
    }

    // 2.2 Load 'module_load_config' from loader
    PyObject* pyLoadingConfFunc =
        PyObject_GetAttrString(pyLoaderModule, PYTHON_API_LOADING_CONF_FUNCNAME);

    if (!pyLoadingConfFunc || !PyCallable_Check(pyLoadingConfFunc)) {
        fprintf(stderr, "Cannot import python module loader function: %s",
                PYTHON_API_LOADING_CONF_FUNCNAME);
        PyErr_Print();
        return NULL;
    }

    // 2.3 Load 'module_execute' from loader
    PyObject* pyExecutorFunc =
        PyObject_GetAttrString(pyLoaderModule, PYTHON_API_EXECUTOR_FUNCNAME);

    if (!pyExecutorFunc || !PyCallable_Check(pyExecutorFunc)) {
        fprintf(stderr, "Cannot import python module executor function: %s",
                PYTHON_API_EXECUTOR_FUNCNAME);
        PyErr_Print();
        return NULL;
    }

    // 3. Create module its own private data
    skullpy::module_data_t* md = (skullpy::module_data_t*)calloc(1, sizeof(*md));
    md->pyLoaderModule     = pyLoaderModule;
    md->pyLoadingFunc      = pyLoadingFunc;
    md->pyLoadingConfFunc  = pyLoadingConfFunc;
    md->pyExecutorFunc     = pyExecutorFunc;
    //printf("loaderModule Ref: %lu, loaderFunc Ref: %lu, pyExecutorFunc Ref: %lu\n",
    //       Py_REFCNT(md->pyLoaderModule), Py_REFCNT(md->pyLoadingFunc),
    //       Py_REFCNT(md->pyExecutorFunc));

    // 4. Call loader module.module_load to load the user module entires
    PyObject* pyArgs = PyTuple_New(1);
    PyObject* pyModuleName = PyString_FromString(module_name);
    PyTuple_SetItem(pyArgs, 0, pyModuleName);

    PyObject* ret = PyObject_CallObject(md->pyLoadingFunc, pyArgs);
    Py_DECREF(pyArgs);

    if (!ret) {
        fprintf(stderr, "Error: Cannot load user module: %s", module_name);
        PyErr_Print();
        free(md);
        return NULL;
    }

    // 4. Assemble execution point
    skull_module_t* module = (skull_module_t*)calloc(1, sizeof(*module));
    module->init    = skullpy::skull_module_init;
    module->run     = skullpy::skull_module_run;
    module->unpack  = skullpy::skull_module_unpack;
    module->pack    = skullpy::skull_module_pack;
    module->release = skullpy::skull_module_release;
    module->ud = md;
    md->name = strdup(module_name);

    PyGILState_Release(state);
    return module;
}

static
int _module_close(skull_module_t* module)
{
    PyGILState_STATE state = PyGILState_Ensure();

    skullpy::module_data_t* md = (skullpy::module_data_t*)module->ud;
    skull_config_destroy(md->config);
    Py_XDECREF(md->pyLoaderModule);
    Py_XDECREF(md->pyLoadingFunc);
    Py_XDECREF(md->pyLoadingConfFunc);
    Py_XDECREF(md->pyExecutorFunc);

    free((void*)md->name);
    free(md);
    free(module);

    PyGILState_Release(state);
    return 0;
}

static
int _module_load_config(skull_module_t* module, const char* filename)
{
    if (!filename) {
        fprintf(stderr, "error: module config name is NULL\n");
        return 1;
    }

    PyGILState_STATE state = PyGILState_Ensure();
    skullpy::module_data_t* md = (skullpy::module_data_t*)module->ud;

    // TODO: Useless, remove it later
    md->config = skull_config_create(filename);

    // Loading config
    PyObject* pyArgs = PyTuple_New(2);
    PyObject* pyModuleName = PyString_FromString(md->name);
    PyObject* pyConfName   = PyString_FromString(filename);
    PyTuple_SetItem(pyArgs, 0, pyModuleName);
    PyTuple_SetItem(pyArgs, 1, pyConfName);

    PyObject* ret = PyObject_CallObject(md->pyLoadingConfFunc, pyArgs);
    Py_DECREF(pyArgs);

    if (!ret) {
        fprintf(stderr, "Error: Cannot load user module(%s) config: %s",
                md->name, filename);
        PyErr_Print();
        PyGILState_Release(state);
        return 1;
    }

    PyGILState_Release(state);
    return 0;
}

static
skull_module_loader_t py_module_getloader() {
    skull_module_loader_t loader;
    loader.name        = _module_name;
    loader.conf_name   = _module_conf_name;
    loader.load_config = _module_load_config;
    loader.open        = _module_open;
    loader.close       = _module_close;

    return loader;
}

namespace skullpy {

// Module Loader Register
void module_loader_register()
{
    skull_module_loader_t loader = py_module_getloader();
    skull_module_loader_register("py", loader);
}

void module_loader_unregister()
{
    skull_module_loader_unregister("py");
}

} // End of namespace

