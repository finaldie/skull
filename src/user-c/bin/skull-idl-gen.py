#!/usr/bin/python

import sys
import os
import getopt
import string
import pprint
import yaml

# global variables
yaml_obj = None
config_name = ""
header_name = ""
source_name = ""

# static
HEADER_CONTENT_START = "\
#ifndef SKULL_IDL_H\n\
#define SKULL_IDL_H\n\
\n\
#include <skull/txn.h>\n\
\n\
"

HEADER_CONTENT_END = "\
\n\
#endif\n\
"

SOURCE_CONTENT_START = "\
#include <string.h>\n\
#include <assert.h>\n\
#include \"%s\"\n\
\n\
"

SOURCE_CONTENT_END = ""

def load_yaml_config():
    global yaml_obj
    global config_name

    yaml_file = file(config_name, 'r')
    yaml_obj = yaml.load(yaml_file)

def _generate_header(idl_name):
    content = ""
    content += "#include \"%s.pb-c.h\"\n" % idl_name
    content += "void* skull_txndata_%s(skull_txn_t*);\n\n" % idl_name

    return content

def generate_header():
    header_file = file(header_name, 'w')
    content = ""

    # generate header
    content += HEADER_CONTENT_START

    # generate body
    idl_tbl = set([])
    idx = -1
    workflows = yaml_obj['workflows']
    for workflow in workflows:
        idx += 1
        if not workflow.get("idl"):
            print "Error: workflow(%d) 'idl' does not exist" % idx
            sys.exit(1)

        idl_name = workflow['idl']
        if idl_name == "":
            print "Error: workflow(%d) idl name is empty" % idx
            sys.exit(1)

        # one idl only generate once
        if idl_name in idl_tbl:
            continue

        idl_tbl.add(idl_name)
        content += _generate_header(idl_name)

    # add desc tbl api
    content += "extern const ProtobufCMessageDescriptor** skull_idl_desc_tbl;\n"

    # generate tailer
    content += HEADER_CONTENT_END

    # write and close the header file
    header_file.write(content)
    header_file.close()

def __generate_txn_data_api(idl_name):
    content = ""
    content += "void* skull_txndata_%s(skull_txn_t* txn) {\n" % idl_name
    content += "    assert(strcmp(skull_txn_idlname(txn), \"%s\"));\n" % idl_name
    content += "    return skull_txn_data(txn);\n"
    content += "}\n\n"

    return content

def _generate_txn_data_api():
    content = ""
    idl_tbl = set([])
    idx = -1
    workflows = yaml_obj['workflows']

    for workflow in workflows:
        idx += 1

        if not workflow.get("idl"):
            print "Error: workflow(%d) 'idl' does not exist" % idx
            sys.exit(1)

        idl_name = workflow['idl']
        if idl_name == "":
            print "Error: workflow(%d) idl name is empty" % idx
            sys.exit(1)

        # one idl only generate once
        if idl_name in idl_tbl:
            continue

        idl_tbl.add(idl_name)
        content += __generate_txn_data_api(idl_name)

    return content

def _generate_source_idl_tbl():
    workflows = yaml_obj['workflows']
    idl_tbl = set([])
    content = "const ProtobufCMessageDescriptor* skull_idl_desc_tbl[] = {\n"

    for workflow in workflows:
        idl_name = workflow['idl']
        if idl_name in idl_tbl:
            continue

        idl_tbl.add(idl_name)
        content += "    &skull__%s__descriptor,\n" % idl_name

    content += "    NULL,\n"
    content += "};\n"
    return content

def generate_source():
    source_file = file(source_name, 'w')
    content = ""

    # generate header
    content += SOURCE_CONTENT_START % header_name

    # generate txn data apis
    content += _generate_txn_data_api()

    # generate idl descriptors table
    content += _generate_source_idl_tbl()

    # generate tailer
    content += SOURCE_CONTENT_END

    # write and close the source file
    source_file.write(content)
    source_file.close()

def generate_idl_files():
    generate_header()
    generate_source()

def usage():
    print "usage: skull-idl-gen.py -c config -h header_file -s source_file"

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'c:h:s:')

        for op, value in opts:
            if op == "-c":
                config_name = value
                load_yaml_config()
            if op == "-h":
                header_name = value
            if op == "-s":
                source_name = value

        # Now run the process func according the mode
        generate_idl_files()

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        sys.exit(1)
