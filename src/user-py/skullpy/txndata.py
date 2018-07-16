# Python Txndata class

import skull_capi

class TxnData(object):
    def __init__(self, skull_txndata):
        self.skull_txndata = skull_txndata

    def append(self, data: bytes):
        skull_capi.txndata_append(self.skull_txndata, data)
        return
