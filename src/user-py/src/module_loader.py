# The global module loader

import sys
import types
import pprint
import skullpy.module_executor as skull_module_executor

# Define required user module entries
MODULE_INIT_FUNCNAME    = 'module_init'
MODULE_RELEASE_FUNCNAME = 'module_release'
MODULE_RUN_FUNCNAME     = 'module_run'
MODULE_UNPACK_FUNCNAME  = 'module_unpack'
MODULE_PACK_FUNCNAME    = 'module_pack'

# Internel APIs
def _load_user_entry(moduleName, entryName, uModule, userModule, isPrintError):
    pkg_modules = uModule.__dict__.get('modules')
    pkg_module  = pkg_modules.__dict__.get(moduleName)
    entry_module = pkg_module.__dict__.get('module')
    func = entry_module.__dict__.get(entryName)

    if isinstance(func, types.FunctionType):
        userModule['entries'][entryName] = func
        return func
    else:
        if isPrintError: print "Error: Not found %s entry" % entryName
        return None

# Public APIs
# Module Loader entry, skull-engine will call this to load a user module
def module_load(module_name):
    print sys.path
    print "module name: %s" % module_name
    full_name = 'skull.modules.' + module_name + '.module'

    # User Module Table
    userModule = {
        'name':     module_name,
        'fullname': full_name,
        'entries':  {}
    }

    # 1. Load user module
    try:
        print "user module for loading: %s" % full_name

        uModule = __import__(full_name)
        #print "uModule: "
        #print uModule
        #pprint(uModule)
    except Exception, e:
        print "Error: Cannot load user module %s: %s" % (module_name, str(e))
        return None

    # 2. Loader user module entries into userModule
    # 2.1 Load module_init
    if _load_user_entry(module_name, MODULE_INIT_FUNCNAME, uModule, userModule, True) is None:
        return None

    # 2.2 Load module_release
    if _load_user_entry(module_name, MODULE_RELEASE_FUNCNAME, uModule, userModule, True) is None:
        return None

    # 2.3 Load module_run
    if _load_user_entry(module_name, MODULE_RUN_FUNCNAME, uModule, userModule, True) is None:
        return None

    # 2.4 Load module_pack
    if _load_user_entry(module_name, MODULE_PACK_FUNCNAME, uModule, userModule, False) is None:
        return None

    # 2.5 Load module_unpack
    if _load_user_entry(module_name, MODULE_UNPACK_FUNCNAME, uModule, userModule, False) is None:
        return None

    return userModule

def module_execute(user_module, entry_name, skull_txn=None, data=None, skull_txndata=None):
    if user_module is None:
        return

    if entry_name is None or isinstance(entry_name, types.StringType) is False:
        return

    entry_func = user_module['entries'][entry_name]

    if entry_name == MODULE_INIT_FUNCNAME:
        skull_module_executor.run_module_init(entry_func)
    elif entry_name == MODULE_RELEASE_FUNCNAME:
        skull_module_executor.run_module_release(entry_func)
    elif entry_name == MODULE_RUN_FUNCNAME:
        return skull_module_executor.run_module_run(entry_func, skull_txn)
    elif entry_name == MODULE_UNPACK_FUNCNAME:
        return skull_module_executor.run_module_unpack(entry_func, skull_txn, data)
    elif entry_name == MODULE_PACK_FUNCNAME:
        skull_module_executor.run_module_pack(entry_func, skull_txn, skull_txndata)

