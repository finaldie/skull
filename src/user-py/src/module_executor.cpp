#include <stdlib.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include <Python.h>
#pragma GCC diagnostic pop

#include <skull/txn.h>
#include "module_loader.h"
#include "module_executor.h"

#define MODULE_INIT_FUNCNAME    "module_init"
#define MODULE_RELEASE_FUNCNAME "module_release"
#define MODULE_RUN_FUNCNAME     "module_run"
#define MODULE_UNPACK_FUNCNAME  "module_unpack"
#define MODULE_PACK_FUNCNAME    "module_pack"

using namespace skullpy;

void   skull_module_init(void* md)
{
    module_data_t* mdata = (module_data_t*)md;

    // Prepare args
    PyObject* pyArgs = PyTuple_New(2);
    PyObject* pyModuleName = PyString_FromString(mdata->name);
    PyObject* pyEntryName  = PyString_FromString(MODULE_INIT_FUNCNAME);

    PyTuple_SetItem(pyArgs, 0, pyModuleName);
    PyTuple_SetItem(pyArgs, 1, pyEntryName);

    // Call user module_init
    PyObject* ret = PyObject_CallObject(mdata->pyExecutorFunc, pyArgs);
    if (!ret) {
        if (PyErr_Occurred()) PyErr_Print();
    }

    Py_XDECREF(ret);
    Py_DECREF(pyArgs);
}

void   skull_module_release(void* md)
{
    module_data_t* mdata = (module_data_t*)md;

    // Prepare args
    PyObject* pyArgs = PyTuple_New(2);
    PyObject* pyModuleName = PyString_FromString(mdata->name);
    PyObject* pyEntryName  = PyString_FromString(MODULE_RELEASE_FUNCNAME);

    PyTuple_SetItem(pyArgs, 0, pyModuleName);
    PyTuple_SetItem(pyArgs, 1, pyEntryName);

    // Call user module_release
    PyObject* ret = PyObject_CallObject(mdata->pyExecutorFunc, pyArgs);
    if (!ret) {
        if (PyErr_Occurred()) PyErr_Print();
    }

    Py_XDECREF(ret);
    Py_DECREF(pyArgs);
}

int    skull_module_run    (void* md, skull_txn_t* txn)
{
    module_data_t* mdata = (module_data_t*)md;

    // Prepare args
    PyObject* pyArgs = PyTuple_New(3);
    PyObject* pyModuleName = PyString_FromString(mdata->name);
    PyObject* pyEntryName  = PyString_FromString(MODULE_RUN_FUNCNAME);
    PyObject* pyTxn        = PyCapsule_New(txn, "skull_txn", NULL);

    PyTuple_SetItem(pyArgs, 0, pyModuleName);
    PyTuple_SetItem(pyArgs, 1, pyEntryName);
    PyTuple_SetItem(pyArgs, 2, pyTxn);

    // Call user module_run
    PyObject* pyRet = PyObject_CallObject(mdata->pyExecutorFunc, pyArgs);
    int ret = 0;

    if (!pyRet) {
        if (PyErr_Occurred()) PyErr_Print();
        ret = 1; // Error occurred
    } else {
        ret = PyObject_IsTrue(pyRet) ? 0 : 1;
    }

    Py_XDECREF(pyRet);
    Py_DECREF(pyArgs);
    return ret;
}

size_t skull_module_unpack (void* md, skull_txn_t* txn,
                            const void* data, size_t data_len)
{
    module_data_t* mdata = (module_data_t*)md;

    // Prepare args
    PyObject* pyArgs = PyTuple_New(4);
    PyObject* pyModuleName = PyString_FromString(mdata->name);
    PyObject* pyEntryName  = PyString_FromString(MODULE_UNPACK_FUNCNAME);
    PyObject* pyTxn        = PyCapsule_New(txn, "skull_txn", NULL);
    PyObject* pyData       = Py_BuildValue("s#", data, data_len); // Read-Only char buffer

    PyTuple_SetItem(pyArgs, 0, pyModuleName);
    PyTuple_SetItem(pyArgs, 1, pyEntryName);
    PyTuple_SetItem(pyArgs, 2, pyTxn);
    PyTuple_SetItem(pyArgs, 3, pyData);

    // Call user module_unpack
    PyObject* pyConsumed = PyObject_CallObject(mdata->pyExecutorFunc, pyArgs);
    size_t consumed = 0;

    if (!pyConsumed) {
        if (PyErr_Occurred()) PyErr_Print();
    } else {
        if (PyInt_Check(pyConsumed)) {
            consumed = PyInt_AsUnsignedLongMask(pyConsumed);
        } else if (PyLong_Check(pyConsumed)) {
            consumed = PyLong_AsUnsignedLongMask(pyConsumed);
        } else {
            fprintf(stderr, "Error: Cannot parse the pyConsumed object");
            _PyObject_Dump(pyConsumed);
        }
    }

    Py_XDECREF(pyConsumed);
    Py_DECREF(pyArgs);
    return consumed;
}

void   skull_module_pack   (void* md, skull_txn_t* txn,
                            skull_txndata_t* txndata)
{
    module_data_t* mdata = (module_data_t*)md;

    // Prepare args
    PyObject* pyArgs = PyTuple_New(4);
    PyObject* pyModuleName = PyString_FromString(mdata->name);
    PyObject* pyEntryName  = PyString_FromString(MODULE_PACK_FUNCNAME);
    PyObject* pyTxn        = PyCapsule_New(txn, "skull_txn", NULL);
    PyObject* pyTxnData    = PyCapsule_New(txndata, "skull_txndata", NULL);
    Py_INCREF(Py_None);

    PyTuple_SetItem(pyArgs, 0, pyModuleName);
    PyTuple_SetItem(pyArgs, 1, pyEntryName);
    PyTuple_SetItem(pyArgs, 2, pyTxn);
    PyTuple_SetItem(pyArgs, 3, Py_None);
    PyTuple_SetItem(pyArgs, 4, pyTxnData);

    // Call user module_pack
    PyObject* pyRet = PyObject_CallObject(mdata->pyExecutorFunc, pyArgs);

    if (!pyRet) {
        if (PyErr_Occurred()) PyErr_Print();
    }

    Py_XDECREF(pyRet);
    Py_DECREF(pyArgs);
}
