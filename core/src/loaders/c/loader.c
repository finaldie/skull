#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "api/sk_loader.h"

typedef struct sk_c_mdata {
    void* handle;
} sk_c_mdata;

sk_module_t* sk_c_module_open(const char* filename)
{
    char* error = NULL;
    void* handle = dlopen(filename, RTLD_LAZY);
    if (!handle) {
        return NULL;
    }

    // create module and its private data
    sk_c_mdata* md = malloc(sizeof(*md));
    md->handle = handle;

    sk_module_t* module = malloc(sizeof(*module));
    module->md = md;

    // load module func
    module->sk_module_init = dlsym(handle, SK_MODULE_INIT_FUNCNAME);
    module->sk_module_run = dlsym(handle, SK_MODULE_RUN_FUNCNAME);

    if ((error = dlerror()) != NULL) {
        return NULL;
    }

    return module;
}

int sk_c_module_close(sk_module_t* module)
{
    sk_c_mdata* md = module->md;
    void* handle = md->handle;

    free(md);
    free(module);
    return dlclose(handle);
}

sk_loader_t sk_c_loader {
    .type = SK_C_MODULE_TYPE,
    .sk_module_open = sk_c_module_open,
    .sk_module_close = sk_c_module_close
};
