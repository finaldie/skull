"""
Python Txndata class
"""

import skull_capi

class TxnData():
    """
    Txn context data interface, send data back to client(peer)
    """
    def __init__(self, skull_txndata):
        self.skull_txndata = skull_txndata

    def append(self, data:bytes):
        """
        Append data into I/O buffer which will be send back to client(peer)
        """
        skull_capi.txndata_append(self.skull_txndata, data)
