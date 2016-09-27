#include <stdlib.h>
#include <google/protobuf/message.h>

#include "mod_loader.h"
#include "srv_loader.h"

// API Layer Register function names
extern "C" {
void skull_load_api();
void skull_unload_api();
}

void skull_load_api()
{
    // 1. Register module loader
    skullcpp::module_loader_register();

    // 2. Register service loader
    skullcpp::service_loader_register();
}

void skull_unload_api()
{
    // 1. Unregister module loader
    skullcpp::module_loader_unregister();

    // 2. Unregister service loader
    skullcpp::service_loader_unregister();

    // 3. Shutdown the protobuf library correctly. Problem: if there are
    //  some dynamic libs used protobuf, if we dlclose them before we calling
    //  the `ShutdownProtobufLibrary`, our program will crash.
    //  Workaround, we ignore the `dlclose`, then we calling the `Shutdown` api.
    google::protobuf::ShutdownProtobufLibrary();
}

