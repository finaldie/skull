#include <stdlib.h>
#include <sys/stat.h>

#include "api/sk_utils.h"
#include "api/sk_loader.h"

extern sk_loader_t* sk_loaders[];

sk_module_t* sk_module_load(const char* short_name)
{
    for (int i = 0; sk_loaders[i] != NULL; i++) {
        sk_loader_t* loader = sk_loaders[i];

        char fullname[1024] = {0};
        loader->sk_module_name(short_name, fullname, 1024);
        sk_module_t* module = loader->sk_module_open(fullname);
        if (module) {
            return module;
        }
    }

    return NULL;
}

void sk_module_unload(sk_module_t* module)
{
    int ret = sk_loaders[module->type]->sk_module_close(module);
    SK_ASSERT_MSG(!ret, "module unload: ret = %d\n", ret);
}

extern sk_loader_t sk_c_loader;

sk_loader_t* sk_loaders[] = {
    &sk_c_loader,       // SK_C_MODULE_TYPE
    NULL
};
