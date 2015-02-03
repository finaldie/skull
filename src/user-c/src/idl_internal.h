#ifndef SKULL_IDL_INTERNAL_H
#define SKULL_IDL_INTERNAL_H

#include <google/protobuf-c/protobuf-c.h>

const ProtobufCMessageDescriptor* skull_idl_descriptor(const char* idl_name);

#endif

