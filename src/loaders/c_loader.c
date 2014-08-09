#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"

typedef struct sk_c_mdata {
    void* handle;
} sk_c_mdata;

sk_module_t* sk_c_module_open(const char* filename,
                              int load_unpack,
                              int load_pack)
{
    char* error = NULL;
    void* handle = dlopen(filename, RTLD_LAZY);
    if (!handle) {
        sk_print("cannot open %s: %s\n", filename, dlerror());
        return NULL;
    }

    // create module and its private data
    sk_c_mdata* md = malloc(sizeof(*md));
    md->handle = handle;

    sk_module_t* module = calloc(1, sizeof(*module));
    module->md = md;

    // load module func
    *(void**)(&module->sk_module_init) = dlsym(handle, SK_MODULE_INIT_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("load %s failed: %s\n", SK_MODULE_INIT_FUNCNAME, error);
        return NULL;
    }

    *(void**)(&module->sk_module_run) = dlsym(handle, SK_MODULE_RUN_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("load %s failed: %s\n", SK_MODULE_RUN_FUNCNAME, error);
        return NULL;
    }

    if (load_unpack) {
        *(void**)(&module->sk_module_unpack) = dlsym(handle,
                                                     SK_MODULE_UNPACK_FUNCNAME);

        if ((error = dlerror()) != NULL) {
            sk_print("load %s failed: %s\n", SK_MODULE_UNPACK_FUNCNAME, error);
            return NULL;
        }
    }

    if (load_pack) {
        *(void**)(&module->sk_module_pack) = dlsym(handle,
                                                   SK_MODULE_PACK_FUNCNAME);

        if ((error = dlerror()) != NULL) {
            sk_print("load %s failed: %s\n", SK_MODULE_PACK_FUNCNAME, error);
            return NULL;
        }
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

sk_loader_t sk_c_loader = {
    .sk_module_open = sk_c_module_open,
    .sk_module_close = sk_c_module_close
};
