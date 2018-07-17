import yaml
import pprint
import sys

from skullpy import txn     as Txn
from skullpy import txndata as TxnData
from skullpy import logger  as Logger
from skullpy import http

from skull.common import protos  as Protos
from skull.common import metrics as Metrics
from skull.common.proto import *

##
# Module Init Entry, be called when start phase
#
# @param config  A parsed yamlObj
#
def module_init(config):
    print "py module init"
    Logger.info('0', 'config: {}'.format(pprint.pformat(config)))

    Logger.trace('py module init: trace test')
    Logger.debug('py module init: debug test')
    Logger.info('1', 'py module init: info test')
    Logger.warn('2', 'py module init: warn test', 'no suggestion')
    Logger.error('3', 'py module init: error test', 'no solution')
    Logger.fatal('4', 'py module init: fatal test', 'no solution')
    return True

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
# @param txn  Transaction context which is used for getting shared transaction
#              data or invoking service `iocall`
# @param data Input data
#
# @return How many bytes be consumed
#
def module_unpack(txn, data):
    #print "py module unpack: {}".format(data)
    #Logger.info('5', 'receive data: {}'.format(data))

    requestHandler = http.Request(data)
    request = None

    try:
        request = requestHandler.parse()
    except http.RequestIncomplete as e:
        print "request body incomplete, need more data: {}".format(e)
        return 0
    except Exception as e:
        print "request parsing failed: {}".format(e)
        return -1

    print "request: {}".format(request)

    # Store data into txn sharedData
    example_msg = txn.data()
    example_msg.data = data
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
    print "py module pack"

    # Increase counters
    mod_metrics = Metrics.module()
    mod_metrics.response.inc(1)

    mod_dymetrics = Metrics.transaction('test')
    mod_dymetrics.response.inc(1)

    # Assemble response
    if txn.status() != Txn.Txn.TXN_OK:
        txndata.append('error')
    else:
        responseHandler = http.Response()
        response = responseHandler.response

        response.status = 200
        response.headerlist = [('Content-type', 'text/html')]
        response.body = 'test'

        res_str = responseHandler.getFullContent()
        txndata.append(res_str)


##
# Module Runnable Entry, be called when this module be picked up in current
#  workflow
#
# @param txn Transaction context
#
# @return - True if no error
#         - False if error occurred
def module_run(txn):
    print "py module run"

    # Increase counters
    mod_metrics = Metrics.module()
    mod_metrics.request.inc(1)

    mod_dymetrics = Metrics.transaction('test')
    mod_dymetrics.request.inc(1)
    return True
