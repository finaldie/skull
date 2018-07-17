#!/usr/bin/python

import sys
import os
import getopt
import string
import pprint
import yaml

header_name = ""
wf_file_list = []
svc_file_list = []
prefix = ""

# static
HEADER_CONTENT_START = "\
#ifndef SKULLCPP_PROTOS_H\n\
#define SKULLCPP_PROTOS_H\n\
\n\
#pragma GCC diagnostic push\n\
#pragma GCC diagnostic ignored \"-Wpadded\"\n\
\n\
"

HEADER_CONTENT_END = "\
\n\
#pragma GCC diagnostic pop\n\
#endif\n\n\
"

def generate_headers(api_file_list):
    content = ""

    if len(api_file_list) == 0:
        return content

    # Generate include contents
    for api_file in api_file_list:
        api_basename = os.path.basename(api_file)
        api_basename = api_basename.split(".")
        api_basename = api_basename[0]

        content += "#include \"%s/%s.pb.h\"\n" % (prefix, api_basename)

    return content

def generate_workflow_header():
    content = "// Workflow transaction shared data protos\n"
    content += generate_headers(wf_file_list)
    return content

def generate_svc_header():
    content = "// Service API protos\n"
    content += generate_headers(svc_file_list)
    return content

def generate_idl_file():
    # Open file
    header_file = file(header_name, 'w')

    # Fill top title
    content = HEADER_CONTENT_START

    # Fill workflow and service header files
    content += generate_workflow_header()
    content += "\n"
    content += generate_svc_header()

    # Fill bottle lines
    content += HEADER_CONTENT_END

    # write and close the source file
    header_file.write(content)
    header_file.close()

def usage():
    print "usage: skull-idl-gen.py -o output_header_file -w workflow_proto_list -s service_proto_list"

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'o:w:s:p:')

        for op, value in opts:
            if op == "-o":
                header_name = value
            elif op == "-w":
                wf_file_list = value.split('|')
                wf_file_list = filter(None, wf_file_list)
            elif op == "-s":
                svc_file_list = value.split('|')
                svc_file_list = filter(None, svc_file_list)
            elif op == "-p":
                prefix = value

        generate_idl_file()

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        sys.exit(1)
