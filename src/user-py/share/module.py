
import yaml

import skullpy.api as api
import skull.common.protos as protos
import skull.common.metrics as metrics

def module_init(config):
    print "py module init"
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
