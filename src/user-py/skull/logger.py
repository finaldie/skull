"""
Python Logger API
"""

import os
import inspect
import skull_capi as capi

def trace(msg):
    """
    Write trace log
    """

    if msg is None:
        return

    if isTraceEnabled() is False:
        return

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    frame = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(frame.filename)

    log_msg = "%s:%d TRACE - %s" % (filename, frame.lineno, str(msg))
    try:
        capi.log(log_msg)
    except Exception as e:
        print("Failed to log message: {}:{} {}".format(filename, frame.lineno, e))

def debug(msg):
    """
    Write debug log
    """

    if msg is None:
        return

    if isDebugEnabled() is False:
        return

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    frame = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(frame.filename)

    log_msg = "%s:%d DEBUG - %s" % (filename, frame.lineno, str(msg))
    try:
        capi.log(log_msg)
    except Exception as e:
        print("Failed to log message: {}:{} {}".format(filename, frame.lineno, e))

def info(code, msg):
    """
    Write info log
    """

    if code is None:
        raise Exception('Logging Format Error: Must have a code')

    if msg is None:
        return

    if isInfoEnabled() is False:
        return

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    frame = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(frame.filename)

    log_msg = "%s:%d INFO - {%s} %s" % (filename, frame.lineno, str(code), str(msg))
    try:
        capi.log(log_msg)
    except Exception as e:
        print("Failed to log message: {}:{} {}".format(filename, frame.lineno, e))

def warn(code, msg, suggestion):
    """
    Write warn log
    """

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
    frame = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(frame.filename)

    log_msg = "%s:%d WARN - {%s} %s; suggestion: %s" % (filename, frame.lineno, \
            str(code), str(msg), str(suggestion))
    try:
        capi.log(log_msg)
    except Exception as e:
        print("Failed to log message: {}:{} {}".format(filename, frame.lineno, e))

def error(code, msg, solution):
    """
    Write error log
    """

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
    frame = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(frame.filename)

    log_msg = "%s:%d ERROR - {%s} %s; solution: %s" % (filename, frame.lineno, \
            str(code), str(msg), str(solution))
    try:
        capi.log(log_msg)
    except Exception as e:
        print("Failed to log message: {}:{} {}".format(filename, frame.lineno, e))

def fatal(code, msg, solution):
    """
    Write fatal log
    """

    if code is None:
        raise Exception('Logging Format Error: Must have a code')

    if msg is None:
        return

    if solution is None:
        raise Exception('Logging Format Error: Must have a solution message')

    caller_frame_record = inspect.stack()[1]
    caller_frame = caller_frame_record[0]
    frame = inspect.getframeinfo(caller_frame)
    filename = os.path.basename(frame.filename)

    log_msg = "%s:%d FATAL - {%s} %s; solution: %s" % (filename, frame.lineno, \
            str(code), str(msg), str(solution))
    try:
        capi.log(log_msg)
    except Exception as e:
        print("Failed to log message: {}:{} {}".format(filename, frame.lineno, e))

# Level Checking APIs
def isTraceEnabled():
    """Return whether trace level enabled"""
    return capi.log_trace_enabled()

def isDebugEnabled():
    """Return whether debug level enabled"""
    return capi.log_debug_enabled()

def isInfoEnabled():
    """Return whether info level enabled"""
    return capi.log_info_enabled()

def isWarnEnabled():
    """Return whether warn level enabled"""
    return capi.log_warn_enabled()

def isErrorEnabled():
    """Return whether error level enabled"""
    return capi.log_error_enabled()

