#ifndef SKULL_IDL_INTERNAL_H
#define SKULL_IDL_INTERNAL_H

#include <google/protobuf-c/protobuf-c.h>

typedef struct skull_idl_data_t {
    void*  data;
    size_t data_sz;
} skull_idl_data_t;

const ProtobufCMessageDescriptor* skull_idl_descriptor(const char* idl_name);

#endif

