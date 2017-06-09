# Skull Module Executor

import skullpy.txn     as Txn
import skullpy.txndata as TxnData
import skullpy.logger  as Logger

def run_module_init(init_func, config):
    init_func(config)

def run_module_release(release_func):
    release_func()

def run_module_run(run_func, skull_txn):
    try:
        txn = Txn.Txn(skull_txn)
        ret = run_func(txn)

        txn.storeMsgData()
        return ret
    except Exception as e:
        Logger.error('module_run', 'module_run failed due to: {}'.format(e),
                'Please check the logic why return False or Exception occurred')
        return False

def run_module_unpack(unpack_func, skull_txn, data):
    txn = Txn.Txn(skull_txn)

    try:
        consumed_length = unpack_func(txn, data)
        txn.storeMsgData()

        if consumed_length is None:
            return -1

        return int(consumed_length)
    except Exception as e:
        Logger.error('module_unpack', 'module_unpack failed due to: {}'.format(e),
                'Please check the logic why return False or Exception occurred')
        return -1 # Error occurred

def run_module_pack(pack_func, skull_txn, skull_txndata):
    txn = Txn.Txn(skull_txn)
    txndata = TxnData.TxnData(skull_txndata)

    try:
        pack_func(txn, txndata)
    except Exception as e:
        Logger.error('module_pack', 'module_pack failed due to: {}'.format(e),
                'Please check the logic why return False or Exception occurred')
    finally:
        txn.destroyMsgData()
