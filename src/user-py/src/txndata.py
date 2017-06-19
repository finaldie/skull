# Python Txndata class

import skull_capi

class TxnData(object):
    def __init__(self, skull_txndata):
        self.skull_txndata = skull_txndata

    def append(self, data):
        skull_capi.txndata_append(self.skull_txndata, bytes(data))
        return
