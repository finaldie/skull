#ifndef SKULL_TXN_H
#define SKULL_TXN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct _skull_txn_t skull_txn_t;

// get idl name from txn
const char* skull_txn_idlname(const skull_txn_t* txn);

typedef enum skull_txn_status_t {
    SKULL_TXN_OK      = 0,
    SKULL_TXN_ERROR   = 1,
    SKULL_TXN_TIMEOUT = 2
} skull_txn_status_t;

// get txn status
skull_txn_status_t skull_txn_status(const skull_txn_t* txn);

void* skull_txn_data(const skull_txn_t* skull_txn);

void skull_txn_setdata(skull_txn_t* skull_txn, const void* data);

// ===================== Txn Iocall ===================
typedef enum skull_txn_ioret_t {
    SKULL_TXN_IO_OK            = 0,
    SKULL_TXN_IO_ERROR_SRVNAME = 1,
    SKULL_TXN_IO_ERROR_APINAME = 2,
    SKULL_TXN_IO_ERROR_STATE   = 3,
    SKULL_TXN_IO_ERROR_BIO     = 4,
    SKULL_TXN_IO_ERROR_SRVBUSY = 5
} skull_txn_ioret_t;

// module callback function declartion
typedef int (*skull_txn_iocb) (skull_txn_t*, skull_txn_ioret_t,
                               const char* service_name,
                               const char* api_name,
                               const void* request, size_t req_sz,
                               const void* response, size_t resp_sz,
                               void* ud);

/**
 * Invoke a service iocall
 *
 * @param serivce_name
 * @param api_name
 * @param request       request protobuf message
 * @param request_sz    request protobuf message size
 * @param cb            api callback function (no pending if it's NULL)
 * @param bio_idx       background io index
 *                      - (-1)  : random pick up a background io to run
 *                      - (0)   : do not use background io
 *                      - (> 0) : run on the index of background io
 * @param ud            user data. This one will be carry over to callback
 *
 * @return - SKULL_TXN_IO_OK
 *         - SKULL_TXN_IO_ERROR_SRVNAME
 *         - SKULL_TXN_IO_ERROR_APINAME
 *         - SKULL_TXN_IO_ERROR_STATE
 *         - SKULL_TXN_IO_ERROR_BIO
 */
skull_txn_ioret_t
skull_txn_iocall (skull_txn_t*,
                  const char* service_name,
                  const char* api_name,
                  const void* request,
                  size_t request_sz,
                  skull_txn_iocb cb,
                  int bio_idx,
                  void* ud);

#ifdef __cplusplus
}
#endif

#endif

