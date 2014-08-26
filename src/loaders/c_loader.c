#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"

#define SK_C_MODULE_FULLNAME_MAXLEN 1024

typedef struct sk_c_mdata {
    void* handle;
} sk_c_mdata;

const char* sk_c_module_name(const char* short_name, char* fullname, int sz)
{
    memset(fullname, 0, sz);
    snprintf(fullname, sz, "modules/%s.so", short_name);
    return fullname;
}

sk_module_t* sk_c_module_open(const char* filename)
{
    // empty all errors first
    dlerror();

    char* error = NULL;
    void* handle = dlopen(filename, RTLD_NOW);
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

    *(void**)(&module->sk_module_unpack) = dlsym(handle,
                                                 SK_MODULE_UNPACK_FUNCNAME);

    if ((error = dlerror()) != NULL) {
        sk_print("warning: load %s failed: %s\n",
                 SK_MODULE_UNPACK_FUNCNAME, error);
    }

    *(void**)(&module->sk_module_pack) = dlsym(handle,
                                               SK_MODULE_PACK_FUNCNAME);

    if ((error = dlerror()) != NULL) {
        sk_print("warning: load %s failed: %s\n",
                 SK_MODULE_PACK_FUNCNAME, error);
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
    .sk_module_name = sk_c_module_name,
    .sk_module_open = sk_c_module_open,
    .sk_module_close = sk_c_module_close
};
