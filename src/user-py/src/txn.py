# Python Txn Class

import types
import skull_capi as capi

from google.protobuf import message
from google.protobuf import message_factory
from google.protobuf import descriptor_pool

# Global message factory (Notes: Should not create it dynamically, or it will lead memleak)
_MESSAGE_FACTORY = message_factory.MessageFactory(descriptor_pool.Default())

class Txn():
    # Txn Status
    TXN_OK      = 0
    TXN_ERROR   = 1
    TXN_TIMEOUT = 2

    # IO Status
    IO_OK            = 0
    IO_ERROR_SVCNAME = 1
    IO_ERROR_APINAME = 2
    IO_ERROR_STATE   = 3
    IO_ERROR_BIO     = 4
    IO_ERROR_SVCBUSY = 5
    IO_ERROR_REQUEST = 6

    def __init__(self, skull_txn):
        self._skull_txn = skull_txn
        self._msg = None

    def data(self):
        idl_name = 'skull.workflow.' + capi.txn_idlname(self._skull_txn)

        return self.__getOrCreateMessage(idl_name)

    def status(self):
        return capi.txn_status(self._skull_txn)

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
    #         - IO_ERROR_SVCNAME
    #         - IO_ERROR_APINAME
    #         - IO_ERROR_STATE
    #         - IO_ERROR_BIO
    #         - IO_ERROR_REQUEST
    def iocall(self, service_name, api_name, request_msg, bio_idx=None, api_cb=None):
        if service_name is None or isinstance(service_name, types.StringType) is False:
            return Txn.IO_ERROR_SVCNAME

        if api_name is None or isinstance(api_name, types.StringType) is False:
            return Txn.IO_ERROR_APINAME

        if request_msg is None or isinstance(request_msg, message.Message) is False:
            return Txn.IO_ERROR_REQUEST

        if api_cb is not None and isinstance(api_cb, types.FunctionType) is False:
            return Txn.IO_ERROR_REQUEST

        if bio_idx is None:
            bio_idx = 0

        msgBinData = request_msg.SerializeToString()
        return capi.txn_iocall(self._skull_txn, service_name, api_name,
                msgBinData, bio_idx, _serviceApiCallback, api_cb)

    # Internal API: Get or Create a message according to full proto name
    def __getOrCreateMessage(self, proto_full_name):
        if self._msg is not None:
            return self._msg

        proto_descriptor = _MESSAGE_FACTORY.pool.FindMessageTypeByName(proto_full_name)
        if proto_descriptor is None: return None

        proto_cls = _MESSAGE_FACTORY.GetPrototype(proto_descriptor)
        self._msg = proto_cls()

        msgBinData = capi.txn_get(self._skull_txn)
        if msgBinData is None:
            return self._msg

        self._msg.ParseFromString(msgBinData)
        return self._msg

    # Store the binary message data back to skull_txn
    def storeMsgData(self):
        if self._msg is None:
            return

        msgBinData = self._msg.SerializeToString()
        capi.txn_set(self._skull_txn, msgBinData)

    # Destroy the binary message data in skull_txn
    def destroyMsgData(self):
        self._msg = None
        capi.txn_set(self._skull_txn, None)

def _serviceApiCallback(skull_txn, io_status, service_name, api_name,
        req_bin_msg, resp_bin_msg, api_cb):
    txn = Txn(skull_txn)

    # Restore request and response message
    req_full_name = 'skull.service.{}.{}_req'.format(service_name, api_name)
    req_msg = _restoreServiceMsg(req_full_name, req_bin_msg)

    resp_full_name = 'skull.service.{}.{}_resp'.format(service_name, api_name)
    resp_msg = _restoreServiceMsg(resp_full_name, resp_bin_msg)

    # Call user callback
    ret = api_cb(txn, io_status, api_name, req_msg, resp_msg)

    txn.storeMsgData()
    return ret

def _restoreServiceMsg(proto_full_name, msg_bin_data):
    proto_descriptor = _MESSAGE_FACTORY.pool.FindMessageTypeByName(proto_full_name)
    if proto_descriptor is None: return None

    proto_cls = _MESSAGE_FACTORY.GetPrototype(proto_descriptor)
    msg = proto_cls()

    if msg_bin_data is None:
        return msg

    msg.ParseFromString(msg_bin_data)
    return msg
