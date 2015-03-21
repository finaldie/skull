#ifndef SKULL_SERVICE_LOADER_H
#define SKULL_SERVICE_LOADER_H

#include <skull/config.h>
#include <skull/service.h>

typedef struct skull_c_srvdata {
    // dll handler
    void* handler;

    // service entry
    skull_service_entry_t* entry;

    // config, it only support 1D format (key: value)
    skull_config_t* config;

    // user layer callback apis
    skull_service_entry_t* (*reg) ();
} skull_c_srvdata;

#endif

