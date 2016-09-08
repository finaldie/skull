
import skullpy.api as api
import skull.common.protos as protos
import skull.common.metrics as metrics

def module_init():
    print "py module init"
    return

def module_release():
    print "py module release"
    return

def module_unpack(skull_txn, data):
    return 0

def module_pack(skull_txn, skull_txndata):
    return

def module_run(skull_txn):
    return True
