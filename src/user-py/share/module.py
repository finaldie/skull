import yaml
import pprint

from skullpy import txn     as Txn
from skullpy import txndata as TxnData
from skullpy import logger  as Logger

from skull.common import protos  as Protos
from skull.common import metrics as Metrics
from skull.common.proto import *

##
# Module Init Entry, be called when start phase
#
# @param config  A parsed yamlObj
#
def module_init(config):
    Logger.info('ModuleInit', 'config: {}'.format(pprint.pformat(config)))
    return

##
# Module Release Function, be called when shutdown phase
#
def module_release():
    Logger.debug("py module release")
    return

##
# Input data unpack function, be called if this module is the 'first' module in
#  the workflow and there is input data incoming
#
# @param txn  Transaction context which is used for getting shared transaction
#              data or invoking service `iocall`
# @param data Input data
#
# @return - > 0: How many bytes consumed
#         - = 0: Need more data
#         - < 0: Error occurred
#
def module_unpack(txn, data):
    Logger.debug('ModuleUnpack: receive data: {}'.format(data))

    # Store data into txn sharedData
    sharedData = txn.data()
    sharedData.data = data
    return len(data)

##
# Input data unpack function, be called if this module is the 'last' module in
#  the workflow (It would no effect if there is no response needed)
#
# @param txn  Transaction context which is used for getting shared transaction
#              data or invoking service `iocall`
# @param data Input data
#
# @return How many bytes be consumed
#
def module_pack(txn, txndata):
    Logger.debug("py module pack")

    # Increase counters
    mod_metrics = Metrics.module()
    mod_metrics.response.inc(1)

    # Assemble response
    if txn.status() != Txn.Txn.TXN_OK:
        txndata.append('error')
    else:
        sharedData = txn.data()
        Logger.debug("pack data: %s" % sharedData.data)
        Logger.info('ModulePack', 'module_pack: data sz: {}'.format(len(sharedData.data)))

        txndata.append(sharedData.data)

##
# Module Runnable Entry, be called when this module be picked up in current
#  workflow
#
# @param txn Transaction context
#
# @return - True if no error
#         - False if error occurred
def module_run(txn):
    Logger.debug("py module run")

    # Increase counters
    mod_metrics = Metrics.module()
    mod_metrics.request.inc(1)
    return True

