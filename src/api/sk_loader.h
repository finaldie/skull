#ifndef SK_MODULE_LOADER_H
#define SK_MODULE_LOADER_H

#include "api/sk_module.h"

#define SK_MODULE_INIT_FUNCNAME   "module_init"
#define SK_MODULE_RUN_FUNCNAME    "module_run"
#define SK_MODULE_UNPACK_FUNCNAME "module_unpack"
#define SK_MODULE_PACK_FUNCNAME   "module_pack"

typedef struct sk_loader_t {
    sk_module_t* (*sk_module_open)(const char* filename,
                                   int load_unpack,
                                   int load_pack);

    int   (*sk_module_close)(sk_module_t* module);
} sk_loader_t;

extern sk_loader_t* sk_loaders[];

#endif

