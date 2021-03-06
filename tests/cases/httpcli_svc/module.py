import yaml
import pprint

from skull import txn     as Txn
from skull import txndata as TxnData
from skull import logger  as Logger

from common import metrics as Metrics
from common.proto import *

##
# Module Init Entry, be called when start phase
#
# @param config  A parsed yamlObj
#
def module_init(config):
    print("py module init")
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
    print("py module release")
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
    print("py module unpack")
    Logger.info('5', 'receive data: {}'.format(data))

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
    print("py module pack")

    # Increase counters
    mod_metrics = Metrics.module()
    mod_metrics.response.inc(1)

    mod_dymetrics = Metrics.transaction('test')
    mod_dymetrics.response.inc(1)

    # Assemble response
    if txn.status() != Txn.Txn.TXN_OK:
        txndata.append('error')
    else:
        sharedData = txn.data()
        print("pack data: %s" % sharedData.data)
        Logger.info('7', 'module_pack: data sz: {}'.format(len(sharedData.data)))

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
    print("py module run")

    # Increase counters
    mod_metrics = Metrics.module()
    mod_metrics.request.inc(1)

    mod_dymetrics = Metrics.transaction('test')
    mod_dymetrics.request.inc(1)

    # Assemble api request
    http_query = service_http_query_req_pto.query_req()
    http_query.isHttps = False
    http_query.method  = 'GET'
    http_query.host    = '216.58.194.196'  # google's ip
    http_query.port    = 80
    http_query.uri     = '/'
    http_query.header.add(name = 'Host', value = 'google.com')
    http_query.timeout = 1000

    # invoke iocall to s1 service
    ret = txn.iocall('http', 'query', http_query, 0, _api_cb)
    print("iocall ret: {}".format(ret))

    return True

def _api_cb(txn, iostatus, api_name, request_msg, response_msg):
    print("api_cb: iostatus: {}, api_name: {}, request_msg: {}, response_msg: {}".format(
            iostatus, api_name, request_msg, response_msg))

    example_msg = txn.data()
    example_msg.data = str(response_msg.body)
    return True
