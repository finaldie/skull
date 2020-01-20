#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "skull/api.h"
#include "skullcpp/module_loader.h"
#include "module_executor.h"
#include "mod_loader.h"

#define SKULL_MODULE_REG_NAME "skullcpp_module_register"

#define SKULL_MODULE_PREFIX_NAME "libskull-modules-"
#define SKULL_MODULE_CONF_PREFIX_NAME "skull-modules-"

static
const char* _module_name(const char* short_name, char* fullname, size_t sz)
{
    memset(fullname, 0, sz);

    // The full name format: lib/libskull-modules-%s.so
    snprintf(fullname, sz, "lib/" SKULL_MODULE_PREFIX_NAME "%s.so", short_name);
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
skull_module_t* _module_open(const char* filename)
{
    // 1. empty all errors first
    dlerror();

    char* error = NULL;
    void* handler = dlopen(filename, RTLD_NOW);
    if (!handler) {
        fprintf(stderr, "Error: Cannot open user module %s: %s\n", filename, dlerror());
        return NULL;
    }

    // 2. create module and its private data
    skullcpp::module_data_t* md = (skullcpp::module_data_t*)calloc(1, sizeof(*md));
    md->handler = handler;

    skull_module_t* module = (skull_module_t*)calloc(1, sizeof(*module));
    module->ud = md;

    // 3. load module entry
    *(void**)(&md->reg) = dlsym(handler, SKULL_MODULE_REG_NAME);
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "Error: Load %s failed: %s\n",
                SKULL_MODULE_REG_NAME, error);
        return NULL;
    }

    md->entry = md->reg();

    module->init    = skullcpp::skull_module_init;
    module->run     = skullcpp::skull_module_run;
    module->unpack  = skullcpp::skull_module_unpack;
    module->pack    = skullcpp::skull_module_pack;
    module->release = skullcpp::skull_module_release;

    return module;
}

static
int _module_close(skull_module_t* module)
{
    skullcpp::module_data_t* md = (skullcpp::module_data_t*)module->ud;
    //void* handler = md->handler;
    skull_config_destroy(md->config);

    free(md);
    free(module);
    //return dlclose(handler);
    return 0;
}

static
int _module_load_config(skull_module_t* module, const char* filename)
{
    if (!filename) {
        fprintf(stderr, "Error: Module config name is NULL\n");
        return 1;
    }

    skullcpp::module_data_t* md = (skullcpp::module_data_t*)module->ud;
    md->config = skull_config_create(filename);
    return 0;
}

namespace skullcpp {

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
void module_loader_register()
{
    skull_module_loader_t loader = module_getloader();
    skull_module_loader_register("cpp", loader);
}

void module_loader_unregister()
{
    skull_module_loader_unregister("cpp");
}

} // End of namespace

