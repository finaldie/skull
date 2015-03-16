#include <stdlib.h>
#include <sys/stat.h>

#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_env.h"
#include "api/sk_log.h"
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
            SK_LOG_INFO(SK_ENV_LOGGER, "module config %s load failed",
                        conf_name);
            return NULL;
        }

        sk_print("load module{%s:%d} successfully\n", short_name, module->type);
        SK_LOG_INFO(SK_ENV_LOGGER, "load service{%s:%d} successfully",
                    short_name, module->type);

        return module;
    }

    return NULL;
}

void sk_module_unload(sk_module_t* module)
{
    int ret = sk_module_loaders[module->type]->close(module);
    SK_ASSERT_MSG(!ret, "module unload failed: ret = %d\n", ret);
}

// =============================================================================

extern sk_service_loader_t sk_c_service_loader;

static sk_service_loader_t* sk_service_loaders[] = {
    &sk_c_service_loader, // array index is SK_C_SERICE_TYPE
    NULL
};

int sk_service_load(sk_service_t* service, const char* conf_name)
{
    const char* service_name = sk_service_name(service);

    for (int i = 0; sk_service_loaders[i] != NULL; i++) {
        sk_service_loader_t* loader = sk_service_loaders[i];

        char fullname[SK_SERVICE_NAME_MAX_LEN] = {0};
        loader->name(service_name, fullname, SK_SERVICE_NAME_MAX_LEN);
        sk_print("try to load service: %s, type: %d - %s\n",
                 service_name, loader->type, fullname);

        sk_service_opt_t service_opt = {NULL, NULL, NULL, NULL};
        int ret = loader->open(fullname, &service_opt);
        if (ret) {
            sk_print("load service: %s failed by loader [%d]\n",
                     service_name, loader->type);
            continue;
        }

        // We successfully load a service, now init other attributes
        sk_service_setopt(service, service_opt);
        sk_service_settype(service, loader->type);

        // load config
        char real_confname[SK_SERVICE_NAME_MAX_LEN] = {0};
        if (!conf_name) {
            conf_name = loader->conf_name(service_name, real_confname,
                                          SK_SERVICE_NAME_MAX_LEN);
        }
        sk_print("service config name: %s\n", conf_name);

        ret = loader->load_config(service, conf_name);
        if (ret) {
            sk_print("service config %s load failed\n", conf_name);
            SK_LOG_INFO(SK_ENV_LOGGER, "service config %s load failed",
                        conf_name);
            return 1;
        }

        sk_print("load service{%s:%d} successfully\n", service_name,
                 sk_service_type(service));

        SK_LOG_INFO(SK_ENV_LOGGER, "load service{%s:%d} successfully",
                    service_name, sk_service_type(service));

        return 0;
    }

    return 1;
}

void sk_service_unload(sk_service_t* service)
{
    sk_service_type_t type = sk_service_type(service);
    int ret = sk_service_loaders[type]->close(service);
    SK_ASSERT_MSG(!ret, "service unload failed: ret = %d\n", ret);
}
