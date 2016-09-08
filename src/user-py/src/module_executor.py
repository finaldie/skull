# Skull Module Executor

def run_module_init(init_func):
    init_func()

def run_module_release(release_func):
    release_func()

def run_module_run(run_func, skull_txn):
    return True

def run_module_unpack(unpack_func, skull_txn, data):
    return 0

def run_module_pack(pack_func, skull_txn, skull_txndata):
    return
