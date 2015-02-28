#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"
#include "api/sk_txn.h"
#include "module_executor.h"

#include "skull/txn.h"
#include "loader.h"

#define SK_MODULE_INIT_FUNCNAME   "module_init"
#define SK_MODULE_RUN_FUNCNAME    "module_run"
#define SK_MODULE_UNPACK_FUNCNAME "module_unpack"
#define SK_MODULE_PACK_FUNCNAME   "module_pack"

#define SK_MODULE_CONFIG_NAME "config.yaml"
#define SK_MODULE_PREFIX_NAME "libskull-modules-"
#define SK_MODULE_CONF_PREFIX_NAME "skull-modules-"

static
const char* _module_name(const char* short_name, char* fullname, size_t sz)
{
    memset(fullname, 0, sz);

    // The full name format: lib/libskull-modules-%s.so
    snprintf(fullname, sz, "lib/" SK_MODULE_PREFIX_NAME "%s.so", short_name);
    return fullname;
}

static
const char* _conf_name(const char* short_name, char* confname, size_t sz)
{
    memset(confname, 0, sz);

    // The full name format: lib/libskull-modules-%s.so
    snprintf(confname, sz, "etc/" SK_MODULE_CONF_PREFIX_NAME "%s.yaml",
             short_name);
    return confname;
}

static
sk_module_t* _module_open(const char* filename)
{
    // 1. empty all errors first
    dlerror();

    char* error = NULL;
    void* handler = dlopen(filename, RTLD_NOW);
    if (!handler) {
        sk_print("error: cannot open %s: %s\n", filename, dlerror());
        return NULL;
    }

    // 2. create module and its private data
    skull_c_mdata* md = calloc(1, sizeof(*md));
    md->handler = handler;

    sk_module_t* module = calloc(1, sizeof(*module));
    module->md = md;

    // 3. load module func
    // 3.1 load init
    *(void**)(&md->init) = dlsym(handler, SK_MODULE_INIT_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("error: load %s failed: %s\n", SK_MODULE_INIT_FUNCNAME, error);
        return NULL;
    }
    module->init = skull_module_init;

    // 3.2 load run
    *(void**)(&md->run) = dlsym(handler, SK_MODULE_RUN_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("error: load %s failed: %s\n", SK_MODULE_RUN_FUNCNAME, error);
        return NULL;
    }
    module->run = skull_module_run;

    // 3.3 load unpack
    *(void**)(&md->unpack) = dlsym(handler, SK_MODULE_UNPACK_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("warning: load %s failed: %s\n",
                 SK_MODULE_UNPACK_FUNCNAME, error);
    }
    module->unpack = skull_module_unpack;

    // 3.4 load pack
    *(void**)(&md->pack) = dlsym(handler, SK_MODULE_PACK_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("warning: load %s failed: %s\n",
                 SK_MODULE_PACK_FUNCNAME, error);
    }
    module->pack = skull_module_pack;

    return module;
}

static
int _module_close(sk_module_t* module)
{
    skull_c_mdata* md = module->md;
    void* handler = md->handler;
    skull_config_destroy(md->config);

    free(md);
    free(module);
    return dlclose(handler);
}

static
int _module_load_config(sk_module_t* module, const char* filename)
{
    if (!filename) {
        sk_print("error: module config name is NULL\n");
        return 1;
    }

    skull_c_mdata* md = module->md;
    md->config = skull_config_create(filename);
    return 0;
}

sk_module_loader_t sk_c_module_loader = {
    .type = SK_C_MODULE_TYPE,
    .name = _module_name,
    .conf_name = _conf_name,
    .load_config = _module_load_config,
    .open = _module_open,
    .close = _module_close
};
