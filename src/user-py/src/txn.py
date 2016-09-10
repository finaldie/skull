# Python Txn Class

import types
import skull_capi
from google.protobuf import message_factory

class Txn():
    # Txn Status
    TXN_OK      = 0
    TXN_ERROR   = 1
    TXN_TIMEOUT = 2

    # IO Status
    IO_OK            = 0
    IO_ERROR_SRVNAME = 1
    IO_ERROR_APINAME = 2
    IO_ERROR_STATE   = 3
    IO_ERROR_BIO     = 4

    def __init__(self, skull_txn):
        self.skull_txn = skull_txn
        self.msg = None

    def data(self):
        idl_name = 'skull.workflow.' + skull_capi.txn_idlname(self.skull_txn)

        return __getOrCreateMessage(idl_name)

    def status(self):
        return skull_capi.txn_status(self.skull_txn)

    ##
    # Send a iocall to service
    #
    # @param service_name
    # @param api_name
    # @param request_msg   protobuf message of this service.api
    # @param bio_idx       background io index
    #                      - (-1)  : random pick up a background io to run
    #                      - (0)   : do not use background io
    #                      - (> 0) : run on the index of background io
    # @param cb            api callback function.
    #                       prototype: func(txn, iostatus, apiName, request_msg, response_msg)
    #
    # @return - IO_OK
    #         - IO_ERROR_SRVNAME
    #         - IO_ERROR_APINAME
    #         - IO_ERROR_STATE
    #         - IO_ERROR_BIO
    def iocall(self, service_name, api_name, request_msg, bio_idx=None, api_cb=None):
        if service_name is None or isinstance(service_name, types.StringType) is False:
            return Txn.IO_ERROR_SRVNAME

        if api_name is None or isinstance(api_name, types.StringType) is False:
            return Txn.IO_ERROR_APINAME

        return skull_capi.txn_iocall(self.skull_txn, service_name, api_name, request_msg, bio_idx, api_cb)

    # Internal API: Get or Create a message according to full proto name
    def __getOrCreateMessage(self, proto_full_name):
        if self.msg is not None:
            return self.msg

        factory = message_factory.MessageFactory(descriptor_pool.Default())
        proto_descriptor = factory.pool.FindMessageTypeByName(proto_full_name)
        if proto_descriptor is None: return None

        proto_cls = factory.GetPrototype(proto_descriptor)
        self.msg = proto_cls()

        msgBinData = skull_capi.txn_get(self.skull_txn)
        if msgBinData is None:
            return self.msg

        self.msg.ParseFromString(msgBinData)
        return self.msg

    # Internel API: Store the binary message data back to skull_txn
    def __storeMsgData(self):
        if self.msg is None:
            return

        msgBinData = self.msg.SerializeToString()
        skull_capi.txn_set(self.skull_txn, msgBinData)

    # Internel API: Destroy the binary message data in skull_txn
    def __destroyMsgData(self):
        self.msg = None
        skull_capi.txn_set(self.skull_txn, None)

