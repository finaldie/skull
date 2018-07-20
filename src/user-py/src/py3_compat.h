#ifndef SKULL_PY3_COMPATIBLE_H
#define SKULL_PY3_COMPATIBLE_H

/**
 * Macros for Python3 compatiblity, add more if needed
 */

# if PY_MAJOR_VERSION >= 3
#  define PyString_FromString        PyUnicode_FromString
#  define PyString_AsString          PyBytes_AS_STRING
#  define PyString_Size              PyBytes_Size
#  define PyString_FromStringAndSize PyBytes_FromStringAndSize
#  define PyInt_Check                PyLong_Check
#  define PyInt_AsSsize_t            PyLong_AsSsize_t
#  define PyInt_FromLong             PyLong_FromLong
# endif

#endif

