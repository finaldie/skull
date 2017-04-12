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

int skull_txn_peer(const skull_txn_t* skull_txn, skull_txn_peer_t* peer)
{
    sk_txn_t* txn = skull_txn->txn;
    sk_entity_t* entity = sk_txn_entity(txn);

    return sk_entity_peer(entity, (sk_entity_peer_t*)peer);
}

skull_txn_peer_type_t skull_txn_peertype(const skull_txn_t* skull_txn)
{
    sk_txn_t* txn = skull_txn->txn;
    sk_entity_t* entity = sk_txn_entity(txn);

    sk_entity_type_t etype = sk_entity_type(entity);
    switch (etype) {
    case SK_ENTITY_NONE:
        return SKULL_TXN_PEER_T_NONE;
    case SK_ENTITY_STD:
        return SKULL_TXN_PEER_T_STD;
    case SK_ENTITY_SOCK_V4TCP:
        return SKULL_TXN_PEER_T_TCPV4;
    case SK_ENTITY_SOCK_V6TCP:
        return SKULL_TXN_PEER_T_TCPV6;
    case SK_ENTITY_SOCK_V4UDP:
        return SKULL_TXN_PEER_T_UDPV4;
    case SK_ENTITY_SOCK_V6UDP:
        return SKULL_TXN_PEER_T_UDPV6;
    default:
        return SKULL_TXN_PEER_T_NONE;
    }
}

