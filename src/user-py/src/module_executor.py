# Skull Module Executor

import skullpy.txn as skullpy_txn
import skullpy.txndata as skullpy_txndata

def run_module_init(init_func):
    init_func()

def run_module_release(release_func):
    release_func()

def run_module_run(run_func, skull_txn):
    txn = skullpy_txn.Txn(skull_txn)
    return run_func(txn)

def run_module_unpack(unpack_func, skull_txn, data):
    txn = skullpy_txn.Txn(skull_txn)
    return unpack_func(txn, data)

def run_module_pack(pack_func, skull_txn, skull_txndata):
    txn = skullpy_txn.Txn(skull_txn)
    txndata = skullpy_txndata.TxnData(skull_txndata)

    pack_func(txn, txndata)
