#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skull/api.h"
#include "skull_metrics.h"
#include "config.h"

void module_init()
{
    printf("module(test): init\n");
    SKULL_LOG_TRACE("skull trace log test %d", 1);
    SKULL_LOG_DEBUG("skull debug log test %d", 2);
    SKULL_LOG_INFO(1, "skull info log test %d", 3);
    SKULL_LOG_WARN(1, "skull warn log test %d", 4);
    SKULL_LOG_ERROR(1, "skull error log test %d", 5);
    SKULL_LOG_FATAL(1, "skull fatal log test %d", 6);

    SKULL_LOG_DEBUG("config test_item: %d, type: %d",
                     skull_config.test_item,
                     skull_config.test_item_type);
}

size_t module_unpack(const void* data, size_t data_sz)
{
    skull_metrics_module.request.inc(1);
    printf("module_unpack(test): data sz:%zu\n", data_sz);
    SKULL_LOG_INFO(1, "module_unpack(test): data sz:%zu", data_sz);
    return data_sz;
}

int module_run(skull_txn_t* txn)
{
    size_t data_sz = 0;
    const char* data = skull_txn_input(txn, &data_sz);
    char* tmp = calloc(1, data_sz + 1);
    memcpy(tmp, data, data_sz);

    printf("receive data: %s\n", tmp);
    SKULL_LOG_INFO(1, "receive data: %s", tmp);
    free(tmp);
    return 0;
}

void module_pack(skull_txn_t* txn)
{
    size_t data_sz = 0;
    const char* data = skull_txn_input(txn, &data_sz);

    skull_metrics_module.response.inc(1);
    printf("module_pack(test): data sz:%zu\n", data_sz);
    SKULL_LOG_INFO(1, "module_pack(test): data sz:%zu", data_sz);
    skull_txn_output_append(txn, data, data_sz);
}
