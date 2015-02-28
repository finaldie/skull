#include <stdlib.h>
#include <sys/stat.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"

extern sk_loader_t* sk_loaders[];

sk_module_t* sk_module_load(const char* short_name, const char* conf_name)
{
    for (int i = 0; sk_loaders[i] != NULL; i++) {
        sk_loader_t* loader = sk_loaders[i];

        char fullname[1024] = {0};
        loader->sk_module_name(short_name, fullname, 1024);
        sk_print("try to load module: %s, type: %d - %s\n",
                 short_name, loader->type, fullname);

        sk_module_t* module = loader->sk_module_open(fullname);
        if (!module) {
            continue;
        }

        // We successfully load a module, now init other attributes
        //  the short_name is the config value, feel free to use it
        module->type = loader->type;
        module->name = short_name;

        // load config
        char real_confname[1024] = {0};
        if (!conf_name) {
            conf_name = loader->conf_name(short_name, real_confname, 1024);
        }
        sk_print("module config name: %s\n", conf_name);

        int ret = loader->module_load_config(module, conf_name);
        if (ret) {
            sk_print("module config %s load failed\n", conf_name);
            return NULL;
        }

        sk_print("load module{%s:%d} successfully\n", short_name, module->type);
        return module;
    }

    return NULL;
}

void sk_module_unload(sk_module_t* module)
{
    int ret = sk_loaders[module->type]->sk_module_close(module);
    SK_ASSERT_MSG(!ret, "module unload: ret = %d\n", ret);
}

extern sk_loader_t sk_c_loader;

sk_loader_t* sk_loaders[] = {
    &sk_c_loader,       // SK_C_MODULE_TYPE
    NULL
};
