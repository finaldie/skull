# Python Client Object
#  User could use it to get the basic information of client

import skull_capi as capi

class Client(object):
    # Client Type
    NONE  = 0
    STD   = 1
    TCPV4 = 2
    TCPV6 = 3
    UDPV4 = 4
    UDPV6 = 5

    # Client Type Name Mapping
    _TYPE_NAME_MAP = {
        NONE:  "NONE",
        STD:   "STD",
        TCPV4: "TCPV4",
        TCPV6: "TCPV6",
        UDPV4: "UDPV4",
        UDPV6: "UDPV6"
    }

    def __init__(self, skull_txn):
        self._skull_txn = skull_txn

        self._name = None
        self._port = -1
        self._type = self.NONE

    def name(self):
        self.__setupPeerInfo()
        return self._name

    def port(self):
        self.__setupPeerInfo()
        return self._port

    def type(self):
        self.__setupPeerInfo()
        return self._type

    def typeName(self):
        self.__setupPeerInfo()

        return self._TYPE_NAME_MAP.get(self._type) or "UNKNOWN"

    def __setupPeerInfo(self):
        if self._name is not None: return

        peer = capi.txn_peer(self._skull_txn)
        self._name = peer[0]
        self._port = peer[1]
        self._type = peer[2]

