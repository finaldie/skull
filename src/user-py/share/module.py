
import yaml

import skullpy.txn as Txn
import skullpy.txndata as TxnData
import skullpy.logger as Logger

import skull.common.protos as protos
import skull.common.metrics as metrics

def module_init(config):
    print "py module init"

    Logger.trace('py module init: trace test')
    Logger.debug('py module init: debug test')
    Logger.info('1', 'py module init: info test')
    Logger.warn('2', 'py module init: warn test', 'no suggestion')
    Logger.error('3', 'py module init: error test', 'no solution')
    Logger.fatal('4', 'py module init: fatal test', 'no solution')
    return

def module_release():
    print "py module release"
    return

def module_unpack(txn, data):
    print "py module unpack"
    return 0

def module_pack(txn, txndata):
    print "py module pack"
    return

def module_run(txn):
    print "py module run"
    return True
