#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#include <Python.h>
#pragma GCC diagnostic pop

#include "module_loader.h"
#include "capis.h"

// API Layer Register function names
extern "C" {
void skull_load_api();
void skull_unload_api();
}

#define PYTHON_SYSPATH1        "/usr/local/lib"
#define PYTHON_SYSPATH2        "/usr/lib"
#define PYTHON_PATH_MAX        (4096)

static
int _append_syspath(const std::string& path)
{
    PyObject* sysPath;
    PyObject* newPath;

    std::string pathPkgName = "path";
    sysPath = PySys_GetObject((char*)pathPkgName.c_str());
    if (!sysPath) {
        return 1;
    }

    newPath = PyString_FromString(path.c_str());
    if (!newPath) {
        return 1;
    }

    if (PyList_Append(sysPath, newPath)) {
        return 1;
    }

    return 0;
}

void skull_load_api()
{
    // 1. Initialize Python Environment
    Py_Initialize();

    // 1.1 Append python loader path into sys.path
    if (_append_syspath(PYTHON_SYSPATH1)) {
        fprintf(stderr, "Error: cannot append python sys.path: %s\n", PYTHON_SYSPATH1);
        exit(1);
    }

    if (_append_syspath(PYTHON_SYSPATH2)) {
        fprintf(stderr, "Error: cannot append python sys.path: %s\n", PYTHON_SYSPATH2);
        exit(1);
    }

    // 1.2 Append python running path into sys.path
    char runningPath[PYTHON_PATH_MAX];
    memset(runningPath, 0, PYTHON_PATH_MAX);
    if (!getcwd(runningPath, PYTHON_PATH_MAX)) {
        fprintf(stderr, "Error: cannot get current path\n");
        exit(1);
    }

    char newRunningPath[PYTHON_PATH_MAX];
    snprintf(newRunningPath, PYTHON_PATH_MAX, "%s/lib/py", runningPath);
    printf("runningPath: %s; newRunningPath: %s\n", runningPath, newRunningPath);

    if (_append_syspath(newRunningPath)) {
        fprintf(stderr, "Error: cannot append python sys.path: %s\n", runningPath);
        exit(1);
    }

    // 2. Register C APIs for Python
    skullpy_register_capis();

    // 3. Register module loader
    skullpy_module_loader_register();
}

void skull_unload_api()
{
    // 1. Unregister module loader
    skullpy_module_loader_unregister();

    // 2. Destroy Python Environment
    Py_Finalize();
}

