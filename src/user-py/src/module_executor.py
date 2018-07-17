# Skull Module Executor

import os
import inspect

import skullpy.txn     as Txn
import skullpy.txndata as TxnData
import skullpy.logger  as Logger

def run_module_init(init_func, config):
    try:
        return init_func(config)
    except Exception as e:
        Logger.error('module_init', 'module_init failed due to: {}'.format(e),
                'StackTrace:\n{}'.format(__dumpStackTrace()))
        return False

def run_module_release(release_func):
    try:
        release_func()
    except Exception as e:
        Logger.error('module_release', 'module_release failed due to: {}'.format(e),
                'StackTrace:\n{}'.format(__dumpStackTrace()))

def run_module_run(run_func, skull_txn):
    try:
        txn = Txn.Txn(skull_txn)
        ret = run_func(txn)

        txn.storeMsgData()
        return ret
    except Exception as e:
        Logger.error('module_run', 'module_run failed due to: {}'.format(e),
                'StackTrace:\n{}'.format(__dumpStackTrace()))
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
                'StackTrace:\n{}'.format(__dumpStackTrace()))
        return -1 # Error occurred

def run_module_pack(pack_func, skull_txn, skull_txndata):
    txn = Txn.Txn(skull_txn)
    txndata = TxnData.TxnData(skull_txndata)

    try:
        pack_func(txn, txndata)
    except Exception as e:
        Logger.error('module_pack', 'module_pack failed due to: {}'.format(e),
                'StackTrace:\n{}'.format(__dumpStackTrace()))
        raise
    finally:
        txn.destroyMsgData()

def __dumpStackTrace():
    output = ""
    caller_frame_records = inspect.trace()
    nRecords = len(caller_frame_records)

    # Print the stack reversed
    for i in range(nRecords - 1, -1, -1):
        record = caller_frame_records[i]
        caller_frame = record[0]
        info = inspect.getframeinfo(caller_frame)

        output += "  at {} ({}:{}) - {}\n".format(info.function, info.filename, info.lineno, str(info.code_context[0]).strip())

    return output

