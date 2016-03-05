#ifndef SKULLCPP_SERVICE_LOADER_H
#define SKULLCPP_SERVICE_LOADER_H

#include <skull/config.h>
#include <skull/service.h>
#include <skull/service_loader.h>
#include <skullcpp/service_loader.h>

namespace skullcpp {

typedef struct srvdata_t {
    // dll handler
    void* handler;

    // service entry
    ServiceEntry* entry;

    // config, it only support 1D format (key: value)
    skull_config_t* config;

    // user layer register api
    ServiceEntry* (*reg) ();
} srvdata_t;

} // End of namespace

// Public Register API
extern "C" {
void skull_service_register();
}

skull_service_loader_t svc_getloader();

#endif
