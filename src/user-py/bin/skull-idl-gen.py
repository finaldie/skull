#!/usr/bin/env python3

import sys
import os
import getopt
import string
import pprint
import yaml

header_file_name = ""
init_file_name = ""
wf_file_list = []
svc_file_list = []
prefix = ""

# static
HEADER_CONTENT_START = "\n"

HEADER_CONTENT_INIT_START = "\
from google.protobuf import descriptor_pb2\n\
from google.protobuf import descriptor_pool\n\
import skullpy.descpool as descpool\n\
\n\
"

HEADER_CONTENT_END = "\
\n\
"

def generate_headers(api_file_list, pkg_name, addReflection):
    content = ""

    if len(api_file_list) == 0:
        return content

    # Generate include contents
    for api_file in api_file_list:
        api_basename = os.path.basename(api_file)
        api_basename = api_basename.split(".")
        api_basename = api_basename[0]
        api_basename = api_basename.replace('-', '_')

        alias_name = "%s_%s_pto" % (pkg_name, api_basename)

        if addReflection is False:
            content += "import skull.common.%s.%s_pb2 as %s\n" % (prefix, api_basename, alias_name)
        else:
            # We use add addReflection = True when generate __init__.py file,
            #  so no need to import the full path
            content += "import %s_pb2 as %s\n" % (api_basename, alias_name)
            content += "descpool.Default().Add(descriptor_pb2.FileDescriptorProto.FromString(%s.DESCRIPTOR.serialized_pb))\n\n" % alias_name

    return content

def generate_workflow_header(addReflection):
    content = "# Workflow transaction shared data protos\n"
    content += generate_headers(wf_file_list, 'workflow', addReflection)
    return content

def generate_svc_header(addReflection):
    content = "# Service API protos\n"
    content += generate_headers(svc_file_list, 'service', addReflection)
    return content

def generate_idl_file(header_file_name):
    if not header_file_name or len(header_file_name) == 0:
        return

    # Open file
    header_file = open(header_file_name, 'w')

    # Fill top title
    content = HEADER_CONTENT_START

    # Fill workflow and service header files
    content += generate_workflow_header(False)
    content += "\n"
    content += generate_svc_header(False)

    # Fill bottle lines
    content += HEADER_CONTENT_END

    # write and close the source file
    header_file.write(content)
    header_file.close()

def generate_init_file(init_file_name):
    if not init_file_name or len(init_file_name) == 0:
        return

    init_file = open(init_file_name, 'w')

    # Fill top title
    content = HEADER_CONTENT_INIT_START

    # Fill workflow and service header files as well as add them into desc
    #  default pool
    content += generate_workflow_header(True)
    content += "\n"
    content += generate_svc_header(True)

    # Fill bottle lines
    content += HEADER_CONTENT_END

    # write and close the source file
    init_file.write(content)
    init_file.close()

def usage():
    print ("usage: skull-idl-gen.py -o output_header_file -i init_file -w workflow_proto_list -s service_proto_list")

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'o:i:w:s:p:')

        for op, value in opts:
            if op == "-o":
                header_file_name = value
            elif op == "-i":
                init_file_name = value
            elif op == "-w":
                wf_file_list = value.split('|')
                wf_file_list = list(filter(None, wf_file_list))
            elif op == "-s":
                svc_file_list = value.split('|')
                svc_file_list = list(filter(None, svc_file_list))
            elif op == "-p":
                prefix = value

        generate_idl_file(header_file_name)
        generate_init_file(init_file_name)

    except Exception as e:
        print ("Fatal: " + str(e))
        usage()
        raise
