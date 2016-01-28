#include <stdlib.h>
#include <dlfcn.h>

#include "flibs/fhash.h"
#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_loader.h"

static fhash* user_libs = NULL;

typedef void (*lib_register) ();

int sk_userlib_load(const char* filename)
{
    if (!user_libs) {
        user_libs = fhash_str_create(0, FHASH_MASK_AUTO_REHASH);
    }

    // Check whether lib has already loaded
    const char* lib = fhash_str_get(user_libs, filename);
    if (lib) {
        sk_print("user lib %s has already loaded\n", filename);
        return 0;
    }

    dlerror();
    char* error = NULL;

    void* handler = dlopen(filename, RTLD_NOW);
    if (!handler) {
        sk_print("error: cannot open %s: %s\n", filename, dlerror());
        return 1;
    }

    // 1. Register module
    lib_register module_register = NULL;
    *(void**)(&module_register) = dlsym(handler, SK_MODULE_REGISTER_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("Error: cannot open user lib %s:%s, reason: %s\n",
                 filename, SK_MODULE_REGISTER_FUNCNAME, error);
        return 1;
    }

    // 2. Run user module register
    module_register();

    // 3. Register service
    lib_register service_register = NULL;
    *(void**)(&service_register) = dlsym(handler, SK_SERVICE_REGISTER_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("Error: cannot open user lib %s:%s, reason: %s\n",
                 filename, SK_SERVICE_REGISTER_FUNCNAME, error);
        return 1;
    }

    // 4. Run user module register
    service_register();

    // 5. Add to userlibs table
    fhash_str_set(user_libs, filename, handler);
    return 0;
}

void sk_userlib_unload()
{
    fhash_str_iter iter = fhash_str_iter_new(user_libs);
    void* handler = NULL;

    while ((handler = fhash_str_next(&iter))) {
        sk_print("Unload user lib %s\n", iter.key);
        dlclose(handler);
    }

    fhash_str_iter_release(&iter);
    fhash_str_delete(user_libs);
    user_libs = NULL;
}
