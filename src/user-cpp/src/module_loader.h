#ifndef SKULLCPP_MODULE_LOADER_H
#define SKULLCPP_MODULE_LOADER_H

#include <skull/api.h>
#include <skullcpp/txn.h>
#include <skullcpp/txndata.h>

typedef struct module_data_t {
    void* handler;
    const skull_config_t* config;

    void*  (*init)    (const skull_config_t*);
    int    (*run)     (skullcpp::Txn& txn);
    size_t (*unpack)  (skullcpp::Txn& txn, const void* data, size_t data_len);
    void   (*pack)    (skullcpp::Txn& txn, skullcpp::TxnData& txndata);
    void   (*release) ();
} module_data_t;

void skullcpp_module_loader_register();
void skullcpp_module_loader_unregister();
skull_module_loader_t module_getloader();

#endif

