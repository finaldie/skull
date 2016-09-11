#include <stdlib.h>

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
    //printf("ParseTuple C metrics inc\n");
    //std::cout << "Tuple Size: " << PyTuple_Size(args) << std::endl;

    if (!PyArg_ParseTuple(args, "sd", &name, &value)) {
        //printf("ParseTuple error\n");
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
PyObject* py_log_trace(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_debug(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_info(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_warn(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_error(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_fatal(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_trace_enabled(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_debug_enabled(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_info_enabled(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_warn_enabled(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_log_error_enabled(PyObject* self, PyObject* args) {
    return NULL;
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
    {"log_trace",         py_log_trace,         METH_VARARGS, "log_trace"},
    {"log_debug",         py_log_debug,         METH_VARARGS, "log_debug"},
    {"log_info",          py_log_info,          METH_VARARGS, "log_info"},
    {"log_warn",          py_log_warn,          METH_VARARGS, "log_warn"},
    {"log_error",         py_log_error,         METH_VARARGS, "log_error"},
    {"log_fatal",         py_log_fatal,         METH_VARARGS, "log_fatal"},
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
