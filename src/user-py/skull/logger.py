"""
Python Logger API
"""

import os
import sys

import skull_capi as capi


def trace(msg):
    """
    Write trace log
    """

    if msg is None:
        return

    if isTraceEnabled() is False:
        return

    fname, lineno = _caller_info()
    log_msg = "%s:%d TRACE - %s" % (fname, lineno, str(msg))
    try:
        capi.log(log_msg)
    except Exception as ex:
        print("Failed to log message: {}:{} {}".format(fname, lineno, ex))


def debug(msg):
    """
    Write debug log
    """

    if msg is None:
        return

    if isDebugEnabled() is False:
        return

    fname, lineno = _caller_info()
    log_msg = "%s:%d DEBUG - %s" % (fname, lineno, str(msg))
    try:
        capi.log(log_msg)
    except Exception as ex:
        print("Failed to log message: {}:{} {}".format(fname, lineno, ex))


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

    fname, lineno = _caller_info()
    log_msg = "%s:%d INFO - {%s} %s" % (fname, lineno, str(code), str(msg))
    try:
        capi.log(log_msg)
    except Exception as ex:
        print("Failed to log message: {}:{} {}".format(fname, lineno, ex))


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

    fname, lineno = _caller_info()
    log_msg = "%s:%d WARN - {%s} %s; suggestion: %s" % (
        fname, lineno, str(code), str(msg), str(suggestion))
    try:
        capi.log(log_msg)
    except Exception as ex:
        print("Failed to log message: {}:{} {}".format(fname, lineno, ex))


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

    fname, lineno = _caller_info()
    log_msg = "%s:%d ERROR - {%s} %s; solution: %s" % (
        fname, lineno, str(code), str(msg), str(solution))
    try:
        capi.log(log_msg)
    except Exception as ex:
        print("Failed to log message: {}:{} {}".format(fname, lineno, ex))


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

    fname, lineno = _caller_info()
    log_msg = "%s:%d FATAL - {%s} %s; solution: %s" % (
        fname, lineno, str(code), str(msg), str(solution))
    try:
        capi.log(log_msg)
    except Exception as ex:
        print("Failed to log message: {}:{} {}".format(fname, lineno, ex))


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


def _caller_info():
    """Return filename and lineno

    This function is trying best to get the frame but not guarantee
    """

    frame = sys._getframe(2) if hasattr(sys, '_getframe') else None
    if frame is None:
        return 'n/a', 0

    return os.path.basename(frame.f_code.co_filename), frame.f_lineno
