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

        // note: since the idl_name is the basic name without namespace, so
        //        have to check whether there is a namespace
        //       for example, the package name is skull, so the full name of
        //       this message is skull.xxx, so the offset is strlen("skull") + 1
        size_t offset = 0;
        size_t package_name_len = strlen(desc->package_name);
        if (package_name_len > 0) {
            offset = package_name_len + 1;
        }

        if (0 == strcmp(desc->name + offset, idl_name)) {
            return desc;
        }
    }

    return NULL;
}
