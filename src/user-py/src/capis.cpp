#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include <Python.h>
#pragma GCC diagnostic pop

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

class ApiCbData {
public:
    PyObject* wrapper_;
    PyObject* userCb_;

public:
    ApiCbData(PyObject* wrapper, PyObject* userCb) : wrapper_(wrapper), userCb_(userCb) {
        Py_XINCREF(this->wrapper_);
        Py_XINCREF(this->userCb_);
    }

    // Do not call Py_DECREF here, or the callback function py obj will be releases
    ~ApiCbData() {
        //Py_XDECREF(this->wrapper_);
        //Py_XDECREF(this->userCb_);
    }
};

// This callback function is trggered by service when the api call has been
//  completed, so here needs to use 'PyGILState_Ensure()' protect python global
//  state again.
static
int _api_cb(skull_txn_t* txn, skull_txn_ioret_t io_status,
            const char* service_name,
            const char* api_name,
            const void* request, size_t req_sz,
            const void* response, size_t resp_sz,
            void* ud) {
    PyGILState_STATE state = PyGILState_Ensure();
    auto* apiCbData = (ApiCbData*)ud;

    PyObject* pyTxn            = PyCapsule_New(txn, "skull_txn", NULL);
    PyObject* pyIoStatus       = PyInt_FromLong(io_status);
    PyObject* pyServiceName    = PyString_FromString(service_name);
    PyObject* pyApiName        = PyString_FromString(api_name);
    PyObject* pyRequestBinMsg  = PyString_FromStringAndSize((const char*)request, (Py_ssize_t)req_sz);
    PyObject* pyResponseBinMsg = PyString_FromStringAndSize((const char*)response, (Py_ssize_t)resp_sz);

    PyObject* pyArgs = PyTuple_New(7);
    PyTuple_SetItem(pyArgs, 0, pyTxn);
    PyTuple_SetItem(pyArgs, 1, pyIoStatus);
    PyTuple_SetItem(pyArgs, 2, pyServiceName);
    PyTuple_SetItem(pyArgs, 3, pyApiName);
    PyTuple_SetItem(pyArgs, 4, pyRequestBinMsg);
    PyTuple_SetItem(pyArgs, 5, pyResponseBinMsg);
    PyTuple_SetItem(pyArgs, 6, apiCbData->userCb_);

    // Call user service callback wrapper
    PyObject* pyRet = PyObject_CallObject(apiCbData->wrapper_, pyArgs);
    int ret = 0;

    if (!pyRet) {
        if (PyErr_Occurred()) PyErr_Print();
        ret = 1; // Error occurred
    } else {
        ret = PyObject_IsTrue(pyRet) ? 0 : 1;
    }

    Py_XDECREF(pyRet);
    Py_DECREF(pyArgs);
    delete apiCbData;
    PyGILState_Release(state);

    // Release the request data
    free((void*)request);
    return ret;
}

static
PyObject* py_txn_iocall(PyObject* self, PyObject* args) {
    const char*  serviceName = NULL;
    const char*  apiName = NULL;
    PyObject*    pyTxn = NULL;
    PyObject*    pyRequestBinMsg = NULL; // serialized request message
    PyObject*    pyCbWrapper = NULL;
    PyObject*    pyApiCb = NULL;
    int          bioIdx = 0; // Default is no background IO
    skull_txn_t* txn = NULL;

    if (!PyArg_ParseTuple(args, "OssO|iOO", &pyTxn, &serviceName, &apiName,
                          &pyRequestBinMsg, &bioIdx, &pyCbWrapper, &pyApiCb)) {
        return NULL;
    }

    if (!pyTxn || pyTxn == Py_None) {
        return NULL;
    }

    txn = (skull_txn_t*) PyCapsule_GetPointer(pyTxn, "skull_txn");
    if (!txn) {
        return NULL;
    }

    void* userCb = NULL;
    if (pyApiCb != Py_None) {
        userCb = new ApiCbData(pyCbWrapper, pyApiCb);
    }

    const char* data = PyString_AsString(pyRequestBinMsg);
    Py_ssize_t size = PyString_Size(pyRequestBinMsg);
    void* serializedMsg = calloc(1, (size_t)size);
    memcpy(serializedMsg, data, (size_t)size);

    skull_txn_ioret_t ret =
        skull_txn_iocall(txn, serviceName, apiName, serializedMsg, (size_t)size,
                         userCb ? _api_cb : NULL, bioIdx, userCb);

    if (ret != SKULL_TXN_IO_OK) {
        delete (ApiCbData*)userCb;
    }

    return Py_BuildValue("i", ret);
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
