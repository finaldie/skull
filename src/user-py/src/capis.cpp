#include <stdlib.h>
#include <stdbool.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include <Python.h>
#pragma GCC diagnostic pop

#include <iostream>

#include "skull/api.h"
#include "capis.h"

static
PyObject* py_txn_idlname(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_txn_get(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_txn_set(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_txn_status(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_txn_iocall(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_txndata_append(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_metrics_inc(PyObject* self, PyObject* args) {
    const char* name = NULL;
    double value = 0.0f;
    //std::cout << "Tuple Size: " << PyTuple_Size(args) << std::endl;

    if (!PyArg_ParseTuple(args, "sd", &name, &value)) {
        return NULL;
    }

    skull_metric_inc(name, value);
    Py_INCREF(Py_None);
    return Py_None;
}

static
PyObject* py_metrics_get(PyObject* self, PyObject* args) {
    const char* name = NULL;
    double value = 0.0f;

    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    value = skull_metric_get(name);
    return Py_BuildValue("d", value);
}

static
PyObject* py_log(PyObject* self, PyObject* args) {
    const char* msg = NULL;

    if (!PyArg_ParseTuple(args, "s", &msg)) {
        return NULL;
    }

    skull_log("%s", msg);
    Py_INCREF(Py_None);
    return Py_None;
}

static
PyObject* py_log_trace_enabled(PyObject* self, PyObject* args) {
    bool enabled = skull_log_enable_trace();

    if (enabled) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static
PyObject* py_log_debug_enabled(PyObject* self, PyObject* args) {
    bool enabled = skull_log_enable_debug();

    if (enabled) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static
PyObject* py_log_info_enabled(PyObject* self, PyObject* args) {
    bool enabled = skull_log_enable_info();

    if (enabled) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static
PyObject* py_log_warn_enabled(PyObject* self, PyObject* args) {
    bool enabled = skull_log_enable_warn();

    if (enabled) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static
PyObject* py_log_error_enabled(PyObject* self, PyObject* args) {
    bool enabled = skull_log_enable_error();

    if (enabled) {
        Py_INCREF(Py_True);
        return Py_True;
    } else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

static PyMethodDef SkullMethods[] = {
    // Txn APIs
    {"txn_idlname",       py_txn_idlname,       METH_VARARGS, "txn_idlname"},
    {"txn_get",           py_txn_get,           METH_VARARGS, "txn_get"},
    {"txn_set",           py_txn_set,           METH_VARARGS, "txn_set"},
    {"txn_status",        py_txn_status,        METH_VARARGS, "txn_status"},
    {"txn_iocall",        py_txn_iocall,        METH_VARARGS, "txn_iocall"},
    {"txndata_append",    py_txndata_append,    METH_VARARGS, "txndata_append"},

    // Metrics APIs
    {"metrics_inc",       py_metrics_inc,       METH_VARARGS, "metrics_inc"},
    {"metrics_get",       py_metrics_get,       METH_VARARGS, "metrics_get"},

    // logger APIs
    {"log",               py_log,               METH_VARARGS, "log"},
    {"log_trace_enabled", py_log_trace_enabled, METH_VARARGS, "log_trace_enabled"},
    {"log_debug_enabled", py_log_debug_enabled, METH_VARARGS, "log_debug_enabled"},
    {"log_info_enabled",  py_log_info_enabled,  METH_VARARGS, "log_info_enabled"},
    {"log_warn_enabled",  py_log_warn_enabled,  METH_VARARGS, "log_warn_enabled"},
    {"log_error_enabled", py_log_error_enabled, METH_VARARGS, "log_error_enabled"},

    {NULL, NULL, 0, NULL}
};

void skullpy_register_capis() {
    Py_InitModule("skull_capi", SkullMethods);
}
