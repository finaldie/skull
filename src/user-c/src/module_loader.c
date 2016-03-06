#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"
#include "api/sk_txn.h"
#include "module_executor.h"

#include "skull/txn.h"
#include "skull/module_loader.h"

#define SKULL_MODULE_REGISTER_NAME "skull_module_register"

static
const char* _module_name(const char* short_name, char* fullname, size_t sz,
                         void* ud)
{
    skull_module_loader_t* loader = ud;
    return loader->name(short_name, fullname, sz);
}

static
const char* _conf_name(const char* short_name, char* confname, size_t sz,
                       void* ud)
{
    skull_module_loader_t* loader = ud;
    return loader->conf_name(short_name, confname, sz);
}

static
sk_module_t* _module_open(const char* filename, void* ud)
{
    skull_module_loader_t* loader = ud;

    // 2. create module and its private data
    skull_module_t* u_module = loader->open(filename);
    if (!u_module) {
        return NULL;
    }

    sk_module_t* module = calloc(1, sizeof(*module));
    module->md      = u_module;
    module->init    = skull_module_init;
    module->run     = skull_module_run;
    module->unpack  = skull_module_unpack;
    module->pack    = skull_module_pack;
    module->release = skull_module_release;

    return module;
}

static
int _module_close(sk_module_t* module, void* ud)
{
    skull_module_loader_t* loader = ud;
    int ret = loader->close(module->md);

    module->md = NULL;
    free(module);
    return ret;
}

static
int _module_load_config(sk_module_t* module, const char* filename, void* ud)
{
    skull_module_loader_t* loader = ud;
    skull_module_t* u_module = module->md;

    if (!filename) {
        sk_print("error: module config name is NULL\n");
        return 1;
    }

    return loader->load_config(u_module, filename);
}

void skull_module_loader_register(const char* type, skull_module_loader_t loader)
{
    skull_module_loader_t* uloader = calloc(1, sizeof(*uloader));
    *uloader = loader;

    sk_module_loader_t sk_module_loader = {
        .name        = _module_name,
        .conf_name   = _conf_name,
        .load_config = _module_load_config,
        .open        = _module_open,
        .close       = _module_close,
        .ud          = uloader
    };

    sk_module_loader_register(type, sk_module_loader);
}

void skull_module_loader_unregister(const char* type)
{
    sk_module_loader_t* loader = sk_module_loader_unregister(type);
    if (!loader) return;

    free(loader->ud);
    free(loader);
}

