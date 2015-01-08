#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"
#include "api/sk_txn.h"
#include "txn_types.h"

#include "skull/txn.h"

#define SK_MODULE_INIT_FUNCNAME   "module_init"
#define SK_MODULE_RUN_FUNCNAME    "module_run"
#define SK_MODULE_UNPACK_FUNCNAME "module_unpack"
#define SK_MODULE_PACK_FUNCNAME   "module_pack"

// Every module has the same file structure:
// module
//  \_ config.yaml
//  \_ mod.so (or mod.lua ...)
#define SK_MODULE_CONFIG_NAME "config.yaml"
#define SK_MODULE_PREFIX_NAME "libskull-modules-"

typedef struct sk_c_mdata {
    void* handle;

    void   (*init)   ();
    int    (*run)    (skull_txn_t* txn);
    size_t (*unpack) (const void* data, size_t data_len);
    void   (*pack)   (skull_txn_t* txn);
} sk_c_mdata;

// skull user module wrapers
static
void _sk_c_module_init(void* md)
{
    sk_c_mdata* mdata = md;
    mdata->init();
}

static
int _sk_c_module_run(void* md, sk_txn_t* txn)
{
    skull_txn_t skull_txn = {
        .txn = txn
    };

    sk_c_mdata* mdata = md;
    return mdata->run(&skull_txn);
}

static
size_t _sk_c_module_unpack(void* md, const void* data, size_t data_len)
{
    sk_c_mdata* mdata = md;
    return mdata->unpack(data, data_len);
}

static
void _sk_c_module_pack(void* md, sk_txn_t* txn)
{
    skull_txn_t skull_txn = {
        .txn = txn
    };

    sk_c_mdata* mdata = md;
    mdata->pack(&skull_txn);
}

const char* sk_c_module_name(const char* short_name, char* fullname, size_t sz)
{
    memset(fullname, 0, sz);

    // The full name like: lib/mod.so
    snprintf(fullname, sz, "lib/" SK_MODULE_PREFIX_NAME "%s.so", short_name);
    return fullname;
}

sk_module_t* sk_c_module_open(const char* filename)
{
    // 1. empty all errors first
    dlerror();

    char* error = NULL;
    void* handle = dlopen(filename, RTLD_NOW);
    if (!handle) {
        sk_print("cannot open %s: %s\n", filename, dlerror());
        return NULL;
    }

    // 2. create module and its private data
    sk_c_mdata* md = calloc(1, sizeof(*md));
    md->handle = handle;

    sk_module_t* module = calloc(1, sizeof(*module));
    module->md = md;

    // 3. load module func
    // 3.1 load init
    *(void**)(&md->init) = dlsym(handle, SK_MODULE_INIT_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("load %s failed: %s\n", SK_MODULE_INIT_FUNCNAME, error);
        return NULL;
    }
    module->init = _sk_c_module_init;

    // 3.2 load run
    *(void**)(&md->run) = dlsym(handle, SK_MODULE_RUN_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("load %s failed: %s\n", SK_MODULE_RUN_FUNCNAME, error);
        return NULL;
    }
    module->run = _sk_c_module_run;

    // 3.3 load unpack
    *(void**)(&md->unpack) = dlsym(handle,
                                                 SK_MODULE_UNPACK_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("warning: load %s failed: %s\n",
                 SK_MODULE_UNPACK_FUNCNAME, error);
    }
    module->unpack = _sk_c_module_unpack;

    // 3.4 load pack
    *(void**)(&md->pack) = dlsym(handle, SK_MODULE_PACK_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("warning: load %s failed: %s\n",
                 SK_MODULE_PACK_FUNCNAME, error);
    }
    module->pack = _sk_c_module_pack;

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
    .type = SK_C_MODULE_TYPE,
    .sk_module_name = sk_c_module_name,
    .sk_module_open = sk_c_module_open,
    .sk_module_close = sk_c_module_close
};
