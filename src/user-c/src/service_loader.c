#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"
#include "api/sk_txn.h"
#include "srv_executor.h"
#include "skull/service_loader.h"

static
const char* _srv_name (const char* short_name, char* fullname, size_t sz,
                       void* ud)
{
    skull_service_loader_t* loader = ud;
    return loader->name(short_name, fullname, sz);
}

static
const char* _srv_conf_name (const char* short_name, char* confname, size_t sz,
                            void* ud)
{
    skull_service_loader_t* loader = ud;
    return loader->conf_name(short_name, confname, sz);
}

static
int _srv_open (const char* filename, sk_service_opt_t* opt/*out*/,
               void* ud)
{
    skull_service_loader_t* loader = ud;
    skull_service_opt_t* u_opt = calloc(1, sizeof(*u_opt));

    int ret = loader->open(filename, u_opt);

    opt->srv_data = u_opt;
    opt->init     = skull_srv_init;
    opt->release  = skull_srv_release;
    opt->iocall   = skull_srv_iocall;
    opt->iocall_complete = skull_srv_iocall_complete;

    return ret;
}

static
int _srv_close (sk_service_t* service, void* ud)
{
    skull_service_loader_t* loader = ud;
    sk_service_opt_t* opt = sk_service_opt(service);
    return loader->close(opt->srv_data);
}

static
int _srv_load_config (sk_service_t* service, const char* filename, void* ud)
{
    if (!filename) {
        sk_print("error: service config name is NULL\n");
        return 1;
    }

    skull_service_loader_t* loader = ud;
    sk_service_opt_t* opt = sk_service_opt(service);
    skull_service_opt_t* u_opt = opt->srv_data;
    return loader->load_config(u_opt, filename);
}

void skull_service_loader_register(const char* type,
                                   skull_service_loader_t loader)
{
    skull_service_loader_t* uloader = calloc(1, sizeof(*uloader));
    *uloader = loader;

    sk_service_loader_t sk_loader = {
        .name        = _srv_name,
        .conf_name   = _srv_conf_name,
        .load_config = _srv_load_config,
        .open        = _srv_open,
        .close       = _srv_close,
        .ud          = uloader
    };

    sk_service_loader_register(type, sk_loader);
}

void skull_service_loader_unregister(const char* type)
{
    sk_service_loader_t* loader = sk_service_loader_unregister(type);
    if (!loader) return;

    free(loader->ud);
    free(loader);
}
