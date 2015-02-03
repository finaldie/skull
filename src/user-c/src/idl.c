#include <stdlib.h>
#include <string.h>

#include "api/sk_workflow.h"
#include "api/sk_utils.h"

#include "skull/idl.h"

static const ProtobufCMessageDescriptor** descriptor_tbl = NULL;

void skull_idl_register(const ProtobufCMessageDescriptor** tbl)
{
    descriptor_tbl = tbl;
}

const ProtobufCMessageDescriptor* skull_idl_descriptor(const char* idl_name)
{
    for (int i = 0; descriptor_tbl[i] != NULL; i++) {
        const ProtobufCMessageDescriptor* desc = descriptor_tbl[i];

        if (0 == strcmp(desc->name, idl_name)) {
            return desc;
        }
    }

    return NULL;
}
