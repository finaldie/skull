#include <stdlib.h>
#include <sys/stat.h>

#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_loader.h"

typedef struct sk_mloader_tbl_t {
    fhash* tbl;
} sk_mloader_tbl_t;

static sk_mloader_tbl_t* mloader_tbl = NULL;

static
sk_mloader_tbl_t* sk_mloader_tbl_create()
{
    sk_mloader_tbl_t* tbl = calloc(1, sizeof(*tbl));
    tbl->tbl = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);

    return tbl;
}

static
void sk_mloader_tbl_destroy(sk_mloader_tbl_t* tbl)
{
    if (!tbl) return;

    // Destroy module loader tbl
    fhash_str_iter iter = fhash_str_iter_new(tbl->tbl);
    sk_module_loader_t* mloader = NULL;

    while ((mloader = fhash_str_next(&iter))) {
        free(mloader);
    }
    fhash_str_iter_release(&iter);
    fhash_str_delete(tbl->tbl);
}

static
sk_module_loader_t* sk_mloader_tbl_get(const char* type)
{
    return fhash_str_get(mloader_tbl->tbl, type);
}

static
void sk_mloader_tbl_set(const char* type, sk_module_loader_t* loader)
{
    fhash_str_set(mloader_tbl->tbl, type, loader);
}

sk_module_t* sk_module_load(const sk_module_cfg_t* cfg,
                            const char* conf_name)
{
    const char* type = cfg->type;
    const char* name = cfg->name;

    sk_module_loader_t* loader = sk_mloader_tbl_get(type);
    if (!loader) {
        return NULL;
    }

    char fullname[SK_MODULE_NAME_MAX_LEN] = {0};
    loader->name(name, fullname, SK_MODULE_NAME_MAX_LEN, loader->ud);
    sk_print("try to load module: %s, type: %s - %s\n",
             name, type, fullname);

    sk_module_t* module = loader->open(fullname, loader->ud);
    if (!module) {
        return NULL;
    }

    // We successfully load a module, now init other attributes
    //  the 'name' is the config value, feel free to use it
    module->cfg = cfg;

    // load config
    char real_confname[SK_MODULE_NAME_MAX_LEN] = {0};
    if (!conf_name) {
        conf_name = loader->conf_name(name, real_confname,
                                      SK_MODULE_NAME_MAX_LEN, loader->ud);
    }
    sk_print("module config name: %s\n", conf_name);

    int ret = loader->load_config(module, conf_name, loader->ud);
    if (ret) {
        sk_print("module config %s load failed\n", conf_name);
        return NULL;
    }

    sk_print("load module{%s:%s} successfully\n", name, type);
    return module;
}

void sk_module_unload(sk_module_t* module)
{
    const sk_module_cfg_t* cfg = module->cfg;
    const char* type = cfg->type;
    sk_module_loader_t* loader = sk_mloader_tbl_get(type);
    SK_ASSERT_MSG(loader, "cannot find loader. type: %s\n", type);

    int ret = loader->close(module, loader->ud);
    SK_ASSERT_MSG(!ret, "module unload failed: ret = %d\n", ret);
}

void sk_module_loader_register(const char* type, sk_module_loader_t loader)
{
    if (!mloader_tbl) {
        mloader_tbl = sk_mloader_tbl_create();
    }

    sk_module_loader_t* mloader = sk_mloader_tbl_get(type);
    if (mloader) return;

    mloader = calloc(1, sizeof(*mloader));
    *mloader = loader;

    sk_mloader_tbl_set(type, mloader);
}

