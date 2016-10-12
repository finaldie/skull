# Python Logger API

import os
import types
import inspect
import skull_capi as capi

def trace(msg):
    if msg is None:
        return

    if isTraceEnabled() is False:
        return

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    info = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(info.filename)

    log_msg = "%s:%d TRACE - %s" % (filename, info.lineno, str(msg))
    try:
        capi.log(log_msg)
    except Exception, e:
        print "Failed to log message: {}:{} {}".format(filename, info.lineno, e)
        pass

def debug(msg):
    if msg is None:
        return

    if isDebugEnabled() is False:
        return

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    info = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(info.filename)

    log_msg = "%s:%d DEBUG - %s" % (filename, info.lineno, str(msg))
    try:
        capi.log(log_msg)
    except Exception, e:
        print "Failed to log message: {}:{} {}".format(filename, info.lineno, e)
        pass

def info(code, msg):
    if code is None:
        raise Exception('Logging Format Error: Must have a code')

    if msg is None:
        return

    if isInfoEnabled() is False:
        return

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    info = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(info.filename)

    log_msg = "%s:%d INFO - {%s} %s" % (filename, info.lineno, str(code), str(msg))
    try:
        capi.log(log_msg)
    except Exception, e:
        print "Failed to log message: {}:{} {}".format(filename, info.lineno, e)
        pass

def warn(code, msg, suggestion):
    if code is None:
        raise Exception('Logging Format Error: Must have a code')

    if msg is None:
        return

    if isWarnEnabled() is False:
        return

    if suggestion is None:
        raise Exception('Logging Format Error: Must have a suggestion message')

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    info = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(info.filename)

    log_msg = "%s:%d WARN - {%s} %s; suggestion: %s" % (filename, info.lineno, str(code), str(msg), str(suggestion))
    try:
        capi.log(log_msg)
    except Exception, e:
        print "Failed to log message: {}:{} {}".format(filename, info.lineno, e)
        pass

def error(code, msg, solution):
    if code is None:
        raise Exception('Logging Format Error: Must have a code')

    if msg is None:
        return

    if isErrorEnabled() is False:
        return

    if solution is None:
        raise Exception('Logging Format Error: Must have a solution message')

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    info = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(info.filename)

    log_msg = "%s:%d ERROR - {%s} %s; solution: %s" % (filename, info.lineno, str(code), str(msg), str(solution))
    try:
        capi.log(log_msg)
    except Exception, e:
        print "Failed to log message: {}:{} {}".format(filename, info.lineno, e)
        pass

def fatal(code, msg, solution):
    if code is None:
        raise Exception('Logging Format Error: Must have a code')

    if msg is None:
        return

    if solution is None:
        raise Exception('Logging Format Error: Must have a solution message')

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    info = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(info.filename)

    log_msg = "%s:%d FATAL - {%s} %s; solution: %s" % (filename, info.lineno, str(code), str(msg), str(solution))
    try:
        capi.log(log_msg)
    except Exception, e:
        print "Failed to log message: {}:{} {}".format(filename, info.lineno, e)
        pass

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

