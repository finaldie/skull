"""
Python Client Object

Below APIs are for getting the basic information of a client
"""

import skull_capi as capi

class Client():
    """
    Client class
    """

    # Client Type
    NONE  = 0
    STD   = 1
    TCPV4 = 2
    TCPV6 = 3
    UDPV4 = 4
    UDPV6 = 5

    """ Client Type Name Mapping """
    _TYPE_NAME_MAP = {
        NONE:  "NONE",
        STD:   "STD",
        TCPV4: "TCPV4",
        TCPV6: "TCPV6",
        UDPV4: "UDPV4",
        UDPV6: "UDPV6"
    }

    def __init__(self, skull_txn):
        """
        Constructor
        """

        self._skull_txn = skull_txn

        self._name = None
        self._port = -1
        self._type = self.NONE

    def name(self):
        """
        name(self) - Return client (peer) name. like: 192.168.1.1
        """

        self.__setup_peer_info()
        return self._name

    def port(self):
        """
        port(self) - Return client (peer) port. like: 80
        """

        self.__setup_peer_info()
        return self._port

    def type(self):
        """
        type(self) - Return client (peer) type. like: TCPV4
        """

        self.__setup_peer_info()
        return self._type

    def typeName(self):
        """
        typeName(self) - Return client (peer) type name. like: 'TCPV4'
        """

        self.__setup_peer_info()

        return self._TYPE_NAME_MAP.get(self._type) or "UNKNOWN"

    def __setup_peer_info(self):
        """
        __setup_peer_info(self) - Internal private method
        """

        if self._name is not None:
            return

        peer = capi.txn_peer(self._skull_txn)
        self._name = peer[0]
        self._port = peer[1]
        self._type = peer[2]

