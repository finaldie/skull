import yaml
import pprint

from skullpy import txn as Txn
from skullpy import txndata as TxnData
from skullpy import logger as Logger

from skull.common import protos as Protos
from skull.common import metrics as Metrics
from skull.common.proto import *

def module_init(config):
    print "py module init"
    print "config: ",
    pprint.pprint(config)

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
    print "data: %s" % data,
    Logger.info('5', 'receive data: {}'.format(data))
    example_msg = txn.data()
    example_msg.data = data

    return len(data)

def module_pack(txn, txndata):
    mod_metrics = Metrics.module()
    mod_metrics.response.inc(1)

    mod_dymetrics = Metrics.transcation('test')
    mod_dymetrics.response.inc(1)

    if txn.status() != Txn.Txn.TXN_OK:
        txndata.append('error')
        Logger.error('6', 'module_pack error', 'no solution')
    else:
        example_msg = txn.data()
        print "pack data: %s" % example_msg.data
        Logger.info('7', 'module_pack: data sz: {}'.format(len(example_msg.data)))

        txndata.append(example_msg.data)

def module_run(txn):
    print "module_run"
    mod_metrics = Metrics.module()
    mod_metrics.request.inc(1)

    mod_dymetrics = Metrics.transcation('test')
    mod_dymetrics.request.inc(1)

    # Assemble api request
    get_req_msg = service_s1_get_req_pto.get_req()
    get_req_msg.name = "hello"

    # invoke iocall to s1 service
    ret = txn.iocall('s1', 'get', get_req_msg, 0, _api_cb)

    print "iocall ret: {}".format(ret)
    return True

def _api_cb(txn, iostatus, api_name, request_msg, response_msg):
    print "api_cb: iostatus: {}, api_name: {}, request_msg: {}, response_msg: {}".format(
            iostatus, api_name, request_msg, response_msg)

    example_msg = txn.data()
    example_msg.data = str(response_msg.response)
    return True

