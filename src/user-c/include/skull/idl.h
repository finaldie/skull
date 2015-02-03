#ifndef SKULL_IDL_H
#define SKULL_IDL_H

#include <google/protobuf-c/protobuf-c.h>

void skull_idl_register(const ProtobufCMessageDescriptor** tbl);

#endif

