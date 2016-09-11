#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include <Python.h>
#pragma GCC diagnostic pop

#include <iostream>

#include "skull/api.h"
#include "txn_idldata.h"
#include "capis.h"

static
PyObject* py_txn_idlname(PyObject* self, PyObject* args) {
    PyObject* pyTxn = NULL;
    skull_txn_t* txn = NULL;

    if (!PyArg_ParseTuple(args, "O", &pyTxn)) {
        return NULL;
    }

    txn = (skull_txn_t*) PyCapsule_GetPointer(pyTxn, "skull_txn");
    if (!txn) {
        return NULL;
    }

    return Py_BuildValue("s", skull_txn_idlname(txn));
}

static
PyObject* py_txn_status(PyObject* self, PyObject* args) {
    PyObject* pyTxn = NULL;
    skull_txn_t* txn = NULL;

    if (!PyArg_ParseTuple(args, "O", &pyTxn)) {
        return NULL;
    }

    txn = (skull_txn_t*) PyCapsule_GetPointer(pyTxn, "skull_txn");
    if (!txn) {
        return NULL;
    }

    return Py_BuildValue("i", skull_txn_status(txn));
}

static
PyObject* py_txn_get(PyObject* self, PyObject* args) {
    PyObject* pyTxn = NULL;
    skull_txn_t* txn = NULL;

    if (!PyArg_ParseTuple(args, "O", &pyTxn)) {
        return NULL;
    }

    txn = (skull_txn_t*) PyCapsule_GetPointer(pyTxn, "skull_txn");
    if (!txn) {
        return NULL;
    }

    const auto* rawData = (skullpy::TxnSharedRawData*)skull_txn_data(txn);
    if (!rawData) {
        Py_INCREF(Py_None);
        return Py_None;
    } else {
        return Py_BuildValue("s#", rawData->data(), rawData->size());
    }
}

static
PyObject* py_txn_set(PyObject* self, PyObject* args) {
    PyObject* pyTxn = NULL;
    skull_txn_t* txn = NULL;
    PyObject* pyData = NULL;

    if (!PyArg_ParseTuple(args, "O|O", &pyTxn, &pyData)) {
        return NULL;
    }

    txn = (skull_txn_t*) PyCapsule_GetPointer(pyTxn, "skull_txn");
    if (!txn) {
        return NULL;
    }

    auto* rawData = (skullpy::TxnSharedRawData*)skull_txn_data(txn);

    if (!pyData || pyData == Py_None) {
        delete rawData;
        skull_txn_setdata(txn, NULL);
    } else {
        if (!rawData) {
            rawData = new skullpy::TxnSharedRawData();
            skull_txn_setdata(txn, rawData);
        }

        const char* data = PyString_AsString(pyData);
        Py_ssize_t size = PyString_Size(pyData);

        if (size > 0) {
            void* newData = calloc(1, (size_t)size);
            memcpy(newData, data, (size_t)size);
            rawData->reset(newData, (size_t)size);
        }
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static
PyObject* py_txn_iocall(PyObject* self, PyObject* args) {
    return NULL;
}

static
PyObject* py_txndata_append(PyObject* self, PyObject* args) {
    const char* buf = NULL;
    int buf_size = 0;
    PyObject* pyTxnData = NULL;
    skull_txndata_t* txndata = NULL;

    if (!PyArg_ParseTuple(args, "Os#", &pyTxnData, &buf, &buf_size)) {
        return NULL;
    }

    if (buf_size <= 0) {
        goto end;
    }

    txndata = (skull_txndata_t*) PyCapsule_GetPointer(pyTxnData, "skull_txndata");
    if (!txndata) {
        return NULL;
    }

    skull_txndata_output_append(txndata, buf, (size_t)buf_size);

end:
    Py_INCREF(Py_None);
    return Py_None;
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
