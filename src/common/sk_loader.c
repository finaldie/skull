#include <stdlib.h>
#include <sys/stat.h>

#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_loader.h"

extern sk_module_loader_t sk_c_module_loader;

static sk_module_loader_t* sk_module_loaders[] = {
    &sk_c_module_loader,       // array index is SK_C_MODULE_TYPE
    NULL
};

sk_module_t* sk_module_load(const char* short_name, const char* conf_name)
{
    for (int i = 0; sk_module_loaders[i] != NULL; i++) {
        sk_module_loader_t* loader = sk_module_loaders[i];

        char fullname[SK_MODULE_NAME_MAX_LEN] = {0};
        loader->name(short_name, fullname, SK_MODULE_NAME_MAX_LEN);
        sk_print("try to load module: %s, type: %d - %s\n",
                 short_name, loader->type, fullname);

        sk_module_t* module = loader->open(fullname);
        if (!module) {
            continue;
        }

        // We successfully load a module, now init other attributes
        //  the short_name is the config value, feel free to use it
        module->type = loader->type;
        module->name = short_name;

        // load config
        char real_confname[SK_MODULE_NAME_MAX_LEN] = {0};
        if (!conf_name) {
            conf_name = loader->conf_name(short_name, real_confname,
                                          SK_MODULE_NAME_MAX_LEN);
        }
        sk_print("module config name: %s\n", conf_name);

        int ret = loader->load_config(module, conf_name);
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
    int ret = sk_module_loaders[module->type]->close(module);
    SK_ASSERT_MSG(!ret, "module unload failed: ret = %d\n", ret);
}

