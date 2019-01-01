"""Skull Module"""

import pprint

from skull import logger
from skull.txn import Txn
from skull.txndata import TxnData

from common import metrics
from common.proto import *


def module_init(config):
    """Module Init Entry.

    It's called once when module be loaded

    @param config  A parsed yamlObj

    @return - True:  Initialization success
            - False: Initialization failure
    """

    logger.info('ModuleInit', 'config: {}'.format(pprint.pformat(config)))
    return True


def module_release():
    """Module Release Function, it's called once when module be unloaded"""

    logger.debug("py module release")


def module_unpack(txn: Txn, data: bytes):
    """Input data unpack function.

    It's called if this module is the 'first' module in the workflow and there
    is input data incoming

    @param txn  Transaction context which is used for getting shared
                transaction data or invoking `service.call()`
    @param data Input read-only `bytes` data

    @return - > 0: Tell engine N bytes be consumed
            - = 0: Need more data
            - < 0: Error occurred
    """

    logger.debug('ModuleUnpack: receive data: {}'.format(data.decode('UTF-8')))

    # Store data into txn sharedData
    sharedata = txn.data()
    sharedata.data = data
    return len(data)


def module_pack(txn: Txn, txndata: TxnData):
    """Output data pack function.

    It's called after 'module_run' function.
     the workflow (It would no effect if there is no response needed)

    @param txn  Transaction context which is used for getting shared
                transaction data or invoking `service.call()`
    @param data Input data

    @return How many bytes be consumed
    """

    logger.debug("py module pack")

    # Increase counters
    mod_metrics = metrics.module()
    mod_metrics.response.inc(1)

    # Assemble response
    if txn.status() != Txn.TXN_OK:
        txndata.append('error')
    else:
        sharedData = txn.data()
        logger.debug("pack data: %s" % sharedData.data)
        logger.info('ModulePack', 'module_pack: data sz: {}'.format(
            len(sharedData.data)))

        txndata.append(sharedData.data)


def module_run(txn: Txn):
    """
    Module Runnable Entry, be called when this module be picked up in current
     workflow

    @param txn Transaction context

    @return - True if no errors
            - False if error occurred
    """

    logger.debug("py module run")

    # Increase counters
    mod_metrics = metrics.module()
    mod_metrics.request.inc(1)
    return True
