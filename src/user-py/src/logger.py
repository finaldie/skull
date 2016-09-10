# Python Logger API

import types
import skull_capi as capi

def trace(msg):
    if msg is None:
        return

    if isTraceEnabled() is False:
        return

    capi.log_trace(str(msg))

def debug(msg):
    if msg is None:
        return

    if isDebugEnabled() is False:
        return

    capi.log_debug(str(msg))

def info(msg):
    if msg is None:
        return

    if isInfoEnabled() is False:
        return

    capi.log_info(str(msg))

def warn(msg, suggestion):
    if msg is None:
        return

    if isWarnEnabled() is False:
        return

    if suggestion is None:
        raise Exception('Logging Format Error: Must have a suggestion message')

    capi.log_warn(str(msg), str(suggestion))

def error(msg, solution):
    if msg is None:
        return

    if isErrorEnabled() is False:
        return

    if solution is None:
        raise Exception('Logging Format Error: Must have a solution message')

    capi.log_error(str(msg), str(solution))

def fatal(msg, solution):
    if msg is None:
        return

    if solution is None:
        raise Exception('Logging Format Error: Must have a solution message')

    capi.log_fatal(str(msg), str(solution))

# Level Checking APIs
def isTraceEnabled():
    return capi.log_trace_enabled()

def isDebugEnabled():
    return capi.log_debug_enabled()

def isInfoEnabled():
    return capi.log_info_enabled()

def isWarnEnabled():
    return capi.log_warn_enabled()

def isErrorEnabled():
    return capi.log_error_enabled()

