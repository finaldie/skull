#ifndef SK_MODULE_LOADER_H
#define SK_MODULE_LOADER_H

#include "api/sk_module.h"

#define SK_MODULE_INIT_FUNCNAME   "module_init"
#define SK_MODULE_RUN_FUNCNAME    "module_run"
#define SK_MODULE_UNPACK_FUNCNAME "module_unpack"
#define SK_MODULE_PACK_FUNCNAME   "module_pack"

typedef struct sk_loader_t {
    const char* (*sk_module_name)(const char* short_name,
                                  char* fullname, int sz);
    sk_module_t* (*sk_module_open)(const char* filename);

    int   (*sk_module_close)(sk_module_t* module);
} sk_loader_t;

/**
 * load a module
 *
 * @note short_name is a short name, for example "test", so it may a C
 * or a cpp/lua/python... module
 *
 * @param[in] short_name    short name of a module
 * @return a module object if success or NULL if failure
 */
sk_module_t* sk_module_load(const char* short_name);

void sk_module_unload(sk_module_t* module);

#endif

