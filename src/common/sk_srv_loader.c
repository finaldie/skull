#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_loader.h"

typedef struct sk_sloader_tbl_t {
    fhash* tbl;
} sk_sloader_tbl_t;

static sk_sloader_tbl_t* sloader_tbl = NULL;

static
sk_sloader_tbl_t* sk_sloader_tbl_create()
{
    sk_sloader_tbl_t* tbl = calloc(1, sizeof(*tbl));
    tbl->tbl = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    return tbl;
}

static
void sk_sloader_tbl_destroy(sk_sloader_tbl_t* tbl)
{
    if (!tbl) return;

    // Destroy module loader tbl
    fhash_str_iter iter = fhash_str_iter_new(tbl->tbl);
    sk_service_loader_t* sloader = NULL;

    while ((sloader = fhash_str_next(&iter))) {
        free(sloader);
    }
    fhash_str_iter_release(&iter);
    fhash_str_delete(tbl->tbl);
}

static
sk_service_loader_t* sk_sloader_tbl_get(const char* type)
{
    return fhash_str_get(sloader_tbl->tbl, type);
}

static
void sk_sloader_tbl_set(const char* type, sk_service_loader_t* loader)
{
    fhash_str_set(sloader_tbl->tbl, type, loader);
}

static
sk_service_loader_t* sk_sloader_tbl_del(const char* type)
{
    return fhash_str_del(sloader_tbl->tbl, type);
}

int sk_service_load(sk_service_t* service, const char* conf_name)
{
    const char* type = sk_service_type(service);
    sk_service_loader_t* loader = sk_sloader_tbl_get(type);
    if (!loader) {
        fprintf(stderr, "cannot find service loader, type: %s\n", type);
        return 1;
    }

    const char* service_name = sk_service_name(service);
    char fullname[SK_SERVICE_NAME_MAX_LEN] = {0};
    loader->name(service_name, fullname, SK_SERVICE_NAME_MAX_LEN, loader->ud);
    sk_print("try to load service: %s, type: %s - %s\n",
             service_name, type, fullname);

    sk_service_opt_t service_opt = {NULL, NULL, NULL, NULL, NULL};
    int ret = loader->open(fullname, &service_opt, loader->ud);
    if (ret) {
        fprintf(stderr, "load service: %s failed by loader [%s]\n",
                 service_name, type);
        return 1;
    }

    // We successfully load a service, now init other attributes
    sk_service_setopt(service, service_opt);

    // load config
    char real_confname[SK_SERVICE_NAME_MAX_LEN] = {0};
    if (!conf_name) {
        conf_name = loader->conf_name(service_name, real_confname,
                                      SK_SERVICE_NAME_MAX_LEN, loader->ud);
    }
    sk_print("service config name: %s\n", conf_name);

    ret = loader->load_config(service, conf_name, loader->ud);
    if (ret) {
        fprintf(stderr, "service config %s load failed\n", conf_name);
        return 1;
    }

    sk_print("load service{%s:%s} successfully\n", service_name,
             sk_service_type(service));
    return 0;
}

void sk_service_unload(sk_service_t* service)
{
    const char* type = sk_service_type(service);
    sk_service_loader_t* loader = sk_sloader_tbl_get(type);
    SK_ASSERT_MSG(loader, "cannot find service loader. type: %s\n", type);

    int ret = loader->close(service, loader->ud);
    SK_ASSERT_MSG(!ret, "service unload failed: ret = %d\n", ret);
}

void sk_service_loader_register(const char* type, sk_service_loader_t loader)
{
    if (!sloader_tbl) {
        sloader_tbl = sk_sloader_tbl_create();
    }

    sk_service_loader_t* sloader = sk_sloader_tbl_get(type);
    if (sloader) return;

    sloader = calloc(1, sizeof(*sloader));
    *sloader = loader;

    sk_sloader_tbl_set(type, sloader);
}

sk_service_loader_t* sk_service_loader_unregister(const char* type)
{
    if (!type) return NULL;
    return sk_sloader_tbl_del(type);
}
