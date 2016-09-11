# Skull Module Executor

import skullpy.txn as Txn
import skullpy.txndata as TxnData

def run_module_init(init_func, config):
    init_func(config)

def run_module_release(release_func):
    release_func()

def run_module_run(run_func, skull_txn):
    txn = Txn.Txn(skull_txn)
    ret = run_func(txn)

    txn.storeMsgData()
    return ret

def run_module_unpack(unpack_func, skull_txn, data):
    txn = Txn.Txn(skull_txn)
    consumed_length = unpack_func(txn, data)

    txn.storeMsgData()
    return consumed_length

def run_module_pack(pack_func, skull_txn, skull_txndata):
    txn = Txn.Txn(skull_txn)
    txndata = TxnData.TxnData(skull_txndata)

    pack_func(txn, txndata)
    txn.destroyMsgData()
