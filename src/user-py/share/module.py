
import yaml

import skullpy.txn as Txn
import skullpy.txndata as TxnData
import skullpy.logger as Logger

import skull.common.protos as protos
import skull.common.metrics as metrics

##
# Module Init Entry, be called when start phase
#
# @param config  A parsed yamlObj
#
def module_init(config):
    print "py module init"

    Logger.trace('py module init: trace test')
    Logger.debug('py module init: debug test')
    Logger.info('1', 'py module init: info test')
    Logger.warn('2', 'py module init: warn test', 'no suggestion')
    Logger.error('3', 'py module init: error test', 'no solution')
    Logger.fatal('4', 'py module init: fatal test', 'no solution')
    return

##
# Module Release Function, be called when shutdown phase
#
def module_release():
    print "py module release"
    return

##
# Input data unpack function, be called if this module is the 'first' module in
#  the workflow and there is input data incoming
#
# @param txn  Transcation context which is used for get shared transcation data,
#              invoke `iocall`
# @param data Input data
#
# @return How many bytes be consumed
#
def module_unpack(txn, data):
    print "py module unpack"

    # Store data into txn sharedData
    example_msg = txn.data()
    example_msg.data = data
    return len(data)

##
# Input data unpack function, be called if this module is the 'last' module in
#  the workflow (It would no effect if there is no response needed)
#
# @param txn  Transcation context which is used for get shared transcation data,
#              invoke `iocall`
# @param data Input data
#
# @return How many bytes be consumed
#
def module_pack(txn, txndata):
    print "py module pack"

    # Increase counters
    mod_metrics = metrics.module()
    mod_metrics.response.inc(1)

    mod_dymetrics = metrics.transcation('test')
    mod_dymetrics.response.inc(1)

    # Assemble response
    example_msg = txn.data()
    print "pack data: %s" % example_msg.data

    txndata.append(example_msg.data)
    return

##
# Module Runnable Entry, be called when this module be picked up in current
#  workflow
#
# @param txn Transcation context
#
# @return - True if no error
#         - False if error occurred
def module_run(txn):
    print "py module run"

    # Increase counters
    mod_metrics = metrics.module()
    mod_metrics.request.inc(1)

    mod_dymetrics = metrics.transcation('test')
    mod_dymetrics.request.inc(1)
    return True
