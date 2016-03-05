#include <stdlib.h>
#include <string.h>
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

    // 1. Open usr api library
    dlerror();
    char* error = NULL;

    char fullname[SK_USER_LIBNAME_MAX];
    memset(fullname, 0, SK_USER_LIBNAME_MAX);
    snprintf(fullname, SK_USER_LIBNAME_MAX, "%s/%s", SK_USER_PREFIX1, filename);

    void* handler = dlopen(fullname, RTLD_NOW);
    if (!handler) {
        sk_print("error: cannot open %s: %s\n", fullname, dlerror());

        memset(fullname, 0, SK_USER_LIBNAME_MAX);
        snprintf(fullname, SK_USER_LIBNAME_MAX, "%s/%s", SK_USER_PREFIX2, filename);
        handler = dlopen(fullname, RTLD_NOW);
        if (!handler) {
            sk_print("error: cannot open %s: %s\n", fullname, dlerror());
            return 1;
        }
    }

    // 2. Find module loader
    lib_register module_register = NULL;
    *(void**)(&module_register) = dlsym(handler, SK_MODULE_REGISTER_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        sk_print("Error: cannot open user lib %s:%s, reason: %s\n",
                 filename, SK_MODULE_REGISTER_FUNCNAME, error);
        return 1;
    }

    // 2.1 Run user module register
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
