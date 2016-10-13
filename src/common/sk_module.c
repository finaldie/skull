#include <stdlib.h>

#include "api/sk_module.h"

size_t sk_module_stat_unpack_inc(sk_module_t* module) {
    if (!module) return 0;

    __sync_fetch_and_add(&module->stat.called_total, 1);
    return __sync_fetch_and_add(&module->stat.called_unpack, 1);
}

size_t sk_module_stat_unpack_get(sk_module_t* module) {
    if (!module) return 0;
    return module->stat.called_unpack;
}

size_t sk_module_stat_pack_inc(sk_module_t* module) {
    if (!module) return 0;
    __sync_fetch_and_add(&module->stat.called_total, 1);
    return __sync_fetch_and_add(&module->stat.called_pack, 1);
}

size_t sk_module_stat_pack_get(sk_module_t* module) {
    if (!module) return 0;
    return module->stat.called_pack;
}

size_t sk_module_stat_run_inc(sk_module_t* module) {
    if (!module) return 0;
    __sync_fetch_and_add(&module->stat.called_total, 1);
    return __sync_fetch_and_add(&module->stat.called_run, 1);
}

size_t sk_module_stat_run_get(sk_module_t* module) {
    if (!module) return 0;
    return module->stat.called_run;
}

size_t sk_module_stat_total_get(sk_module_t* module) {
    if (!module) return 0;
    return module->stat.called_total;
}

