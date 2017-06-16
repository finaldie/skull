#include <stdlib.h>

#include "api/sk_module.h"

size_t sk_module_stat_inc_unpack(sk_module_t* module) {
    if (!module) return 0;
    return __sync_fetch_and_add(&module->stat.unpack, 1);
}

size_t sk_module_stat_inc_pack(sk_module_t* module) {
    if (!module) return 0;
    return __sync_fetch_and_add(&module->stat.pack, 1);
}

size_t sk_module_stat_inc_run(sk_module_t* module) {
    if (!module) return 0;
    return __sync_fetch_and_add(&module->stat.run, 1);
}

