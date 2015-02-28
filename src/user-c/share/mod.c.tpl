#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skull/api.h"
#include "skull_metrics.h"
#include "skull_idl.h"
#include "config.h"

void* module_init(skull_config_t* config)
{
    printf("module(test): init\n");
    SKULL_LOG_TRACE("skull trace log test %d", 1);
    SKULL_LOG_DEBUG("skull debug log test %d", 2);
    SKULL_LOG_INFO(1, "skull info log test %d", 3);
    SKULL_LOG_WARN(1, "skull warn log test %d", 4);
    SKULL_LOG_ERROR(1, "skull error log test %d", 5);
    SKULL_LOG_FATAL(1, "skull fatal log test %d", 6);

    // For c module, must register the idl table during the initialization
    skull_idl_register(skull_idl_desc_tbl);

    // Convert skull_config to skull_static_config
    skull_static_config_convert(config);

    SKULL_LOG_DEBUG("config test_item: %d", skull_static_config()->test_item);
    SKULL_LOG_DEBUG("config test_rate: %f", skull_static_config()->test_rate);
    SKULL_LOG_DEBUG("config test_name: %s", skull_static_config()->test_name);

    return NULL;
}

void module_release(void* user_data)
{
    skull_static_config_destroy();
}

size_t module_unpack(skull_txn_t* txn, const void* data, size_t data_sz)
{
    skull_metrics_module.request.inc(1);
    printf("module_unpack(test): data sz:%zu\n", data_sz);
    SKULL_LOG_INFO(1, "module_unpack(test): data sz:%zu", data_sz);

    // deserialize data to transcation data
    Skull__Example* example = skull_idldata_example(txn);
    example->data.len = data_sz;
    example->data.data = calloc(1, data_sz);
    memcpy(example->data.data, data, data_sz);

    return data_sz;
}

int module_run(skull_txn_t* txn)
{
    Skull__Example* example = skull_idldata_example(txn);
    size_t data_sz = example->data.len;
    const char* data = (const char*)example->data.data;

    char* tmp = calloc(1, data_sz + 1);
    memcpy(tmp, data, data_sz);

    printf("receive data: %s\n", tmp);
    SKULL_LOG_INFO(1, "receive data: %s", tmp);

    free(tmp);
    return 0;
}

void module_pack(skull_txn_t* txn, skull_txndata_t* txndata)
{
    Skull__Example* example = skull_idldata_example(txn);
    size_t data_sz = example->data.len;
    const char* data = (const char*)example->data.data;

    skull_metrics_module.response.inc(1);
    printf("module_pack(test): data sz:%zu\n", data_sz);
    SKULL_LOG_INFO(1, "module_pack(test): data sz:%zu", data_sz);
    skull_txndata_output_append(txndata, data, data_sz);
}
