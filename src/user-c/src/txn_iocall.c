#include <stdlib.h>

#include "api/sk_env.h"
#include "txn_utils.h"
#include "skull/txn.h"

skull_txn_ioret_t
skull_txn_iocall (skull_txn_t* txn, const char* service_name,
                  const char* api_name, const void* request,
                  size_t request_sz, skull_txn_iocb cb, int bidx, void* ud)
{
    sk_core_t* core = SK_ENV_CORE;
    sk_service_t* service = sk_core_service(core, service_name);
    if (!service) {
        return SKULL_TXN_IO_ERROR_SRVNAME;
    }

    // find the exact async api
    const sk_service_api_t* api = sk_service_api(service, api_name);
    if (!api) {
        return SKULL_TXN_IO_ERROR_APINAME;
    }

    if (sk_txn_state(txn->txn) == SK_TXN_INIT) {
        return SKULL_TXN_IO_ERROR_STATE;
    }


    // serialize the request data
    int ioret = sk_service_iocall(service, txn->txn, api_name,
                      request, request_sz, (sk_txn_task_cb)cb, ud, bidx);

    if (ioret) {
        return SKULL_TXN_IO_ERROR_BIO;
    }

    return SKULL_TXN_IO_OK;
}
