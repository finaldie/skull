#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"
#include "api/sk_txn.h"
#include "srv_executor.h"

#include "skull/txn.h"
#include "srv_loader.h"

#define SKULL_SRV_REG_NAME "skull_service_register"

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
int _srv_open (const char* filename, sk_service_opt_t* opt/*out*/)
{
    // 1. empty all errors first
    dlerror();

    char* error = NULL;
    void* handler = dlopen(filename, RTLD_NOW);
    if (!handler) {
        sk_print("error: cannot open %s: %s\n", filename, dlerror());
        return 1;
    }

    // 2. create service and its private data
    skull_c_srvdata* md = calloc(1, sizeof(*md));
    md->handler = handler;

    // 3. load service register func
    *(void**)(&md->reg) = dlsym(handler, SKULL_SRV_REG_NAME);
    if ((error = dlerror()) != NULL) {
        sk_print("error: load %s failed: %s\n", SKULL_SRV_REG_NAME, error);
        return 1;
    }

    md->entry = md->reg();

    opt->srv_data = md;
    opt->mode = SK_SRV_DATA_MODE_EXCLUSIVE;
    opt->init = skull_srv_init;
    opt->release = skull_srv_release;
    opt->io_call = skull_srv_iocall;

    return 0;
}

static
int _srv_close (sk_service_t* service)
{
    sk_service_opt_t* opt = sk_service_opt(service);
    skull_c_srvdata* srv_data = opt->srv_data;
    void* handler = srv_data->handler;
    skull_config_destroy(srv_data->config);

    free(srv_data);
    return dlclose(handler);
}

static
int _srv_load_config (sk_service_t* service, const char* filename)
{
    if (!filename) {
        sk_print("error: service config name is NULL\n");
        return 1;
    }

    sk_service_opt_t* opt = sk_service_opt(service);
    skull_c_srvdata* srv_data = opt->srv_data;
    srv_data->config = skull_config_create(filename);
    return 0;
}

sk_service_loader_t sk_c_service_loader = {
    .type        = SK_C_SERVICE_TYPE,
    .name        = _srv_name,
    .conf_name   = _srv_conf_name,
    .load_config = _srv_load_config,
    .open        = _srv_open,
    .close       = _srv_close
};
