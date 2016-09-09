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
    if (!ret && PyErr_Occurred()) {
        PyErr_Print();
    }

    Py_DECREF(pyArgs);
}

void   skull_module_release(void* md)
{
    module_data_t* mdata = (module_data_t*)md;

    // Prepare args
    PyObject* pyArgs = PyTuple_New(2);
    PyObject* pyModuleName = PyString_FromString(mdata->name);
    PyObject* pyEntryName = PyString_FromString(MODULE_RELEASE_FUNCNAME);

    PyTuple_SetItem(pyArgs, 0, pyModuleName);
    PyTuple_SetItem(pyArgs, 1, pyEntryName);

    // Call user module_init
    PyObject* ret = PyObject_CallObject(mdata->pyExecutorFunc, pyArgs);
    if (!ret && PyErr_Occurred()) {
        PyErr_Print();
    }

    Py_DECREF(pyArgs);
}

int    skull_module_run    (void* md, skull_txn_t* txn)
{
    //// 1. deserialize the binary data to user layer structure
    //skullcpp::TxnImp uTxn(txn);

    //// 2. run module
    //module_data_t* mdata = (module_data_t*)md;
    //ModuleEntry* entry = mdata->entry;

    //int ret = entry->run(uTxn);
    //return ret;
    return 0;
}

size_t skull_module_unpack (void* md, skull_txn_t* txn,
                            const void* data, size_t data_len)
{
    //// 1. prepare a fresh new user layer idl structure
    //skullcpp::TxnImp uTxn(txn);

    //// 2. run unpack
    //module_data_t* mdata = (module_data_t*)md;
    //ModuleEntry* entry = mdata->entry;

    //size_t consumed_sz = entry->unpack(uTxn, data, data_len);
    //return consumed_sz;
    return 0;
}

void   skull_module_pack   (void* md, skull_txn_t* txn,
                            skull_txndata_t* txndata)
{
    //// 1. Create txn and txndata
    //skullcpp::TxnImp uTxn(txn, true);
    //skullcpp::TxnDataImp uTxnData(txndata);

    //// 2. run pack
    //module_data_t* mdata = (module_data_t*)md;
    //ModuleEntry* entry = mdata->entry;

    //entry->pack(uTxn, uTxnData);
}
