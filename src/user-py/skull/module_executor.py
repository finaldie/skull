"""
Skull Module Executor
"""

import inspect

import skull.txn     as Txn
import skull.txndata as TxnData
import skull.logger  as Logger

def run_module_init(init_func, config):
    """
    Execute 'module init' entry function
    """
    try:
        return init_func(config)
    except Exception as e:
        Logger.error('module_init', 'module_init failed due to: {}'.format(e), \
                'StackTrace:\n{}'.format(__dumpStackTrace()))
        return False

def run_module_release(release_func):
    """
    Execute 'module release' entry function
    """
    try:
        release_func()
    except Exception as e:
        Logger.error('module_release', 'module_release failed due to: {}'.format(e), \
                'StackTrace:\n{}'.format(__dumpStackTrace()))

def run_module_run(run_func, skull_txn):
    """
    Execute 'module run' entry function
    """
    try:
        txn = Txn.Txn(skull_txn)
        ret = run_func(txn)

        txn.storeMsgData()
        return ret
    except Exception as e:
        Logger.error('module_run', 'module_run failed due to: {}'.format(e), \
                'StackTrace:\n{}'.format(__dumpStackTrace()))
        return False

def run_module_unpack(unpack_func, skull_txn, data: bytes):
    """
    Execute 'module unpack' entry function
    """
    txn = Txn.Txn(skull_txn)

    try:
        consumed_length = unpack_func(txn, data)
        txn.storeMsgData()

        if consumed_length is None:
            return -1

        return int(consumed_length)
    except Exception as e:
        Logger.error('module_unpack', 'module_unpack failed due to: {}'.format(e), \
                'StackTrace:\n{}'.format(__dumpStackTrace()))
        return -1 # Error occurred

def run_module_pack(pack_func, skull_txn, skull_txndata):
    """
    Execute 'module pack' entry function
    """
    txn = Txn.Txn(skull_txn)
    txndata = TxnData.TxnData(skull_txndata)

    try:
        pack_func(txn, txndata)
    except Exception as e:
        Logger.error('module_pack', 'module_pack failed due to: {}'.format(e), \
                'StackTrace:\n{}'.format(__dumpStackTrace()))
        raise
    finally:
        txn.destroyMsgData()

def __dumpStackTrace():
    """
    Internal stack trace dumper
    """
    output = ""
    caller_frame_records = inspect.trace()
    nrecords = len(caller_frame_records)

    # Print the stack reversed
    for i in range(nrecords - 1, -1, -1):
        record = caller_frame_records[i]
        caller_frame = record[0]
        info = inspect.getframeinfo(caller_frame)

        output += "  at {} ({}:{}) - {}\n".format(info.function, info.filename, \
                info.lineno, str(info.code_context[0]).strip())

    return output

