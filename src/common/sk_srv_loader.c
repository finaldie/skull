#include <stdlib.h>
#include <sys/stat.h>

#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_loader.h"

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
            return 1;
        }

        sk_print("load service{%s:%d} successfully\n", service_name,
                 sk_service_type(service));
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
