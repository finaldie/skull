# The global module loader

import os
import sys
import types
import pprint

import yaml
import skullpy.module_executor as skull_module_executor

# Define required user module entries
MODULE_INIT_FUNCNAME    = 'module_init'
MODULE_RELEASE_FUNCNAME = 'module_release'
MODULE_RUN_FUNCNAME     = 'module_run'
MODULE_UNPACK_FUNCNAME  = 'module_unpack'
MODULE_PACK_FUNCNAME    = 'module_pack'

# Global Module Table
UserModuleTables = {}

# Global Environment Variables
EnvVars = None

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

    # Create Global Environment Vars
    global EnvVars
    if EnvVars is None:
        EnvVars = {
            'working_dir': os.getcwd()
        }

    # User Module Table
    userModule = {
        'env':          EnvVars,
        'name':         module_name,
        'fullname':     full_name,
        'module_obj':   None,
        'config':       None,
        'config_name:': '',
        'entries':      {}
    }

    # 1. Load user module
    try:
        print "user module for loading: %s" % full_name

        uModule = __import__(full_name)
    except Exception, e:
        print "Error: Cannot load user module %s: %s" % (module_name, str(e))
        raise

    userModule['module_obj'] = uModule

    # 2. Loader user module entries into userModule
    # 2.1 Load module_init
    if _load_user_entry(module_name, MODULE_INIT_FUNCNAME, uModule, userModule, True) is None:
        return False

    # 2.2 Load module_release
    if _load_user_entry(module_name, MODULE_RELEASE_FUNCNAME, uModule, userModule, True) is None:
        return False

    # 2.3 Load module_run
    if _load_user_entry(module_name, MODULE_RUN_FUNCNAME, uModule, userModule, True) is None:
        return False

    # 2.4 Load module_pack
    if _load_user_entry(module_name, MODULE_PACK_FUNCNAME, uModule, userModule, False) is None:
        return False

    # 2.5 Load module_unpack
    if _load_user_entry(module_name, MODULE_UNPACK_FUNCNAME, uModule, userModule, False) is None:
        return False

    # 3. Assembe user module into Global Module Table
    UserModuleTables[module_name] = userModule
    return True

def module_load_config(module_name, config_file_name):
    if module_name is None or isinstance(module_name, types.StringType) is False:
        return

    if config_file_name is None or isinstance(config_file_name, types.StringType) is False:
        return

    user_module = UserModuleTables.get(module_name)
    if user_module is None:
        print "Cannot find user module: %s" % module_name
        raise

    # Loading the config yaml file
    yaml_file = None

    try:
        yaml_file = file(config_file_name, 'r')
    except Exception, e:
        print "Error: Cannot load user module %s config %s : %s" % (module_name, config_file_name, str(e))
        raise

    conf_yml_obj = yaml.load(yaml_file)
    user_module['config']      = conf_yml_obj
    user_module['config_name'] = config_file_name
    yaml_file.close()

def module_execute(module_name, entry_name, skull_txn=None, data=None, skull_txndata=None):
    if module_name is None or isinstance(module_name, types.StringType) is False:
        return

    if entry_name is None or isinstance(entry_name, types.StringType) is False:
        return

    user_module = UserModuleTables.get(module_name)
    if user_module is None:
        print "Cannot find user module: %s" % module_name
        return

    entry_func = user_module['entries'][entry_name]

    if entry_name == MODULE_INIT_FUNCNAME:
        config = user_module['config']
        skull_module_executor.run_module_init(entry_func, config)
    elif entry_name == MODULE_RELEASE_FUNCNAME:
        skull_module_executor.run_module_release(entry_func)
    elif entry_name == MODULE_RUN_FUNCNAME:
        return skull_module_executor.run_module_run(entry_func, skull_txn)
    elif entry_name == MODULE_UNPACK_FUNCNAME:
        return skull_module_executor.run_module_unpack(entry_func, skull_txn, data)
    elif entry_name == MODULE_PACK_FUNCNAME:
        skull_module_executor.run_module_pack(entry_func, skull_txn, skull_txndata)
    else:
        print "Unknown entry name for execution: %s" % entry_name

