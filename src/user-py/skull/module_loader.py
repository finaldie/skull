"""
Global module loader
"""

import os
import sys
import types
import importlib

import yaml
import skull.module_executor as skull_module_executor
import skull.logger as Logger

# Define required user module entries
MODULE_INIT_FUNCNAME    = 'module_init'
MODULE_RELEASE_FUNCNAME = 'module_release'
MODULE_RUN_FUNCNAME     = 'module_run'
MODULE_UNPACK_FUNCNAME  = 'module_unpack'
MODULE_PACK_FUNCNAME    = 'module_pack'

# Global Module Table
USER_MODULE_TABLES = {}

# Global Environment Variables
ENV_VARS = None

# Internel APIs
def _load_user_entry(module_name, entry_name, umodule, user_module, is_print_error):
    func = umodule.__dict__.get(entry_name)

    if isinstance(func, types.FunctionType):
        user_module['entries'][entry_name] = func
        return func
    else:
        if is_print_error:
            Logger.fatal('user_entry', 'Module {}: Not found entry {}'.format( \
                module_name, entry_name), 'Please check the source file')

        return None

# Public APIs
def module_load(module_name):
    """
    Module Loader entry point, engine will call this API to load a user module
    """

    #print (sys.path, file=sys.stderr)
    #print ("module name: %s" % module_name, file=sys.stderr)
    full_name = 'modules.%s.module' % module_name

    # Create Global Environment Vars
    global ENV_VARS
    if ENV_VARS is None:
        ENV_VARS = {
            'working_dir': os.getcwd()
        }

    # User Module Table
    user_module = {
        'env'         : ENV_VARS,
        'name'        : module_name,
        'fullname'    : full_name,
        'module_obj'  : None,
        'config'      : None,
        'config_name:': '',
        'entries'     : {}
    }

    # 1. Load user module
    umodule = None
    try:
        #print ("Loading user module: %s" % full_name, file=sys.stderr)

        umodule = importlib.import_module(full_name)
    except Exception as e:
        Logger.fatal('module_load', 'Cannot load user module {}: {}'.format( \
                module_name, str(e)), 'please check the module whether exist')
        raise

    user_module['module_obj'] = umodule

    # 2. Loader user module entries into user_module
    # 2.1 Load module_init
    if _load_user_entry(module_name, MODULE_INIT_FUNCNAME, umodule, user_module, True) is None:
        return False

    # 2.2 Load module_release
    if _load_user_entry(module_name, MODULE_RELEASE_FUNCNAME, umodule, user_module, True) is None:
        return False

    # 2.3 Load module_run
    if _load_user_entry(module_name, MODULE_RUN_FUNCNAME, umodule, user_module, True) is None:
        return False

    # 2.4 Load module_pack
    _load_user_entry(module_name, MODULE_PACK_FUNCNAME, umodule, user_module, False)

    # 2.5 Load module_unpack
    _load_user_entry(module_name, MODULE_UNPACK_FUNCNAME, umodule, user_module, False)

    # 3. Assembe user module into Global Module Table
    USER_MODULE_TABLES[module_name] = user_module
    return True

def module_load_config(module_name, config_file_name):
    """
    Engine call this API to load a user module's config file
    """
    if module_name is None or isinstance(module_name, str) is False:
        return

    if config_file_name is None or isinstance(config_file_name, str) is False:
        return

    user_module = USER_MODULE_TABLES.get(module_name)
    if user_module is None:
        Logger.fatal('load_config', 'Cannot find user module: {}'.format(module_name), \
                'please check the module has contained correct entries')
        raise

    # Loading the config yaml file
    yaml_file = None

    try:
        yaml_file = open(config_file_name, 'r')
    except Exception as e:
        Logger.fatal('load_config', \
            'Cannot load user module {} config {} : {}'.format(module_name, \
            config_file_name, str(e)), 'please check the config content format')
        raise

    conf_yml_obj = yaml.load(yaml_file)
    user_module['config']      = conf_yml_obj
    user_module['config_name'] = config_file_name
    yaml_file.close()

def module_execute(module_name, entry_name, skull_txn=None, data=None, skull_txndata=None):
    """
    Engine call this API to execute a user module callbacks

    Callbacks are: module_init, module_release, module_run, module_unpack, module_pack
    """
    if module_name is None or isinstance(module_name, str) is False:
        return None

    if entry_name is None or isinstance(entry_name, str) is False:
        return None

    user_module = USER_MODULE_TABLES.get(module_name)
    if user_module is None:
        Logger.fatal('module_execute', 'Cannot find user module: {}'.format( \
            module_name), 'please check the module has contained correct entries')
        return None

    entry_func = user_module['entries'][entry_name]

    ret = None
    if entry_name == MODULE_INIT_FUNCNAME:
        config = user_module['config']
        ret = skull_module_executor.run_module_init(entry_func, config)
    elif entry_name == MODULE_RELEASE_FUNCNAME:
        skull_module_executor.run_module_release(entry_func)
    elif entry_name == MODULE_RUN_FUNCNAME:
        ret = skull_module_executor.run_module_run(entry_func, skull_txn)
    elif entry_name == MODULE_UNPACK_FUNCNAME:
        ret = skull_module_executor.run_module_unpack(entry_func, skull_txn, data)
    elif entry_name == MODULE_PACK_FUNCNAME:
        skull_module_executor.run_module_pack(entry_func, skull_txn, skull_txndata)
    else:
        Logger.error('module_execute', 'Unknown entry name for execution: {}'.format( \
            entry_name), 'Please check the engine code, it shouldn\'t be')

    return ret

