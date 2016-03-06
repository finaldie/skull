#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "srv_executor.h"

#include "skull/txn.h"
#include "skull/service_loader.h"
#include "srv_loader.h"

#define SKULL_SRV_REG_NAME "skullcpp_service_register"

#define SKULL_SRV_PREFIX_NAME "libskull-services-"
#define SKULL_SRV_CONF_PREFIX_NAME "skull-services-"

static
const char* _srv_name (const char* short_name, char* fullname, size_t sz)
{
    memset(fullname, 0, sz);

    // The full name format: lib/libskull-services-%s.so
    snprintf(fullname, sz, "lib/" SKULL_SRV_PREFIX_NAME "%s.so", short_name);
    return fullname;
}

static
const char* _srv_conf_name (const char* short_name, char* confname, size_t sz)
{
    memset(confname, 0, sz);

    // The full name format: etc/skull-services-%s.yaml
    snprintf(confname, sz, "etc/" SKULL_SRV_CONF_PREFIX_NAME "%s.yaml",
             short_name);
    return confname;
}

static
int _srv_open (const char* filename, skull_service_opt_t* opt/*out*/)
{
    // 1. empty all errors first
    dlerror();

    char* error = NULL;
    void* handler = dlopen(filename, RTLD_NOW);
    if (!handler) {
        fprintf(stderr, "error: cannot open %s: %s\n", filename, dlerror());
        return 1;
    }

    // 2. create service and its private data
    skullcpp::srvdata_t* md = (skullcpp::srvdata_t*)calloc(1, sizeof(*md));
    md->handler = handler;

    // 3. load service register func
    *(void**)(&md->reg) = dlsym(handler, SKULL_SRV_REG_NAME);
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "error: load %s failed: %s\n", SKULL_SRV_REG_NAME, error);
        return 1;
    }

    md->entry = md->reg();

    opt->ud      = md;
    opt->init    = skull_srv_init;
    opt->release = skull_srv_release;
    opt->iocall  = skull_srv_iocall;
    opt->iocomplete = skull_srv_iocomplete;

    return 0;
}

static
int _srv_close (skull_service_opt_t* opt)
{
    skullcpp::srvdata_t* srv_data = (skullcpp::srvdata_t*)opt->ud;
    //void* handler = srv_data->handler;
    skull_config_destroy(srv_data->config);

    free(srv_data);
    //return dlclose(handler);
    return 0;
}

static
int _srv_load_config (skull_service_opt_t* opt, const char* filename)
{
    if (!filename) {
        fprintf(stderr, "error: service config name is NULL\n");
        return 1;
    }

    skullcpp::srvdata_t* srv_data = (skullcpp::srvdata_t*)opt->ud;
    srv_data->config = skull_config_create(filename);
    return 0;
}

skull_service_loader_t svc_getloader()
{
    skull_service_loader_t loader;
    loader.name        = _srv_name;
    loader.conf_name   = _srv_conf_name;
    loader.load_config = _srv_load_config;
    loader.open        = _srv_open;
    loader.close       = _srv_close;

    return loader;
}

// Service Loader Register
void skullcpp_service_loader_register()
{
    skull_service_loader_register("cpp", svc_getloader());
}

void skullcpp_service_loader_unregister()
{
    skull_service_loader_unregister("cpp");
}

