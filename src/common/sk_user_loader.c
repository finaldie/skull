#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "flibs/fhash.h"
#include "api/sk_utils.h"
#include "api/sk_const.h"
#include "api/sk_loader.h"

typedef struct sk_user_api_t {
    void* handler;
    void (*load) ();
    void (*unload) ();
} sk_user_api_t;

static fhash* user_libs = NULL;

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

    void* handler = dlopen(fullname, RTLD_NOW | RTLD_GLOBAL);
    if (!handler) {
        fprintf(stdout, "Notice: cannot open %s: %s\n", fullname, dlerror());

        memset(fullname, 0, SK_USER_LIBNAME_MAX);
        snprintf(fullname, SK_USER_LIBNAME_MAX, "%s/%s", SK_USER_PREFIX2, filename);
        handler = dlopen(fullname, RTLD_NOW);
        if (!handler) {
            fprintf(stderr, "Error: cannot open %s: %s\n", fullname, dlerror());
            return 1;
        }
    }

    sk_user_api_t* user_api = calloc(1, sizeof(*user_api));
    user_api->handler = handler;

    // 2. Find api loader
    *(void**)(&user_api->load) = dlsym(handler, SK_API_LOAD_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "Error: cannot setup user 'load' api %s:%s, reason: %s\n",
                 filename, SK_API_LOAD_FUNCNAME, error);
        return 1;
    }

    // 3. Find api unloader
    *(void**)(&user_api->unload) = dlsym(handler, SK_API_UNLOAD_FUNCNAME);
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "Error: cannot setup user 'unload' api %s:%s, reason: %s\n",
                 filename, SK_API_UNLOAD_FUNCNAME, error);
        return 1;
    }

    // 4. Add to userlibs table
    fhash_str_set(user_libs, filename, user_api);

    // 5. Run api loader
    user_api->load();
    return 0;
}

void sk_userlib_unload()
{
    fhash_str_iter iter = fhash_str_iter_new(user_libs);
    sk_user_api_t* user_api = NULL;

    while ((user_api = fhash_str_next(&iter))) {
        sk_print("Unload user lib %s\n", iter.key);
        user_api->unload();

        // Close handler may cause some issues, e.g. crash on calling
        //  `protobuf::shutdown` api
        //dlclose(user_api->handler);
        free(user_api);
    }

    fhash_str_iter_release(&iter);
    fhash_str_delete(user_libs);
    user_libs = NULL;
}
