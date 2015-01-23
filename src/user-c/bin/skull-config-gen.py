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
HEADER_NO_CFG_ITEM = "\
    char _nothing[24];\n\
    skull_cfg_itype_t _nothing_type;\n\n\
"

SOURCE_NO_CFG_ITEM = "\
    ._nothing = \"there is no config item\",\n\
    ._nothing_type = SKULL_CFG_ITYPE_STRING,\n\
"

HEADER_CONTENT_START = "\
#ifndef SKULL_CONFIG_C_H\n\
#define SKULL_CONFIG_C_H\n\
\n\
#include <skull/config.h>\n\
\n\
#pragma pack(4)\n\
typedef struct skull_config_t {\
\n"

HEADER_CONTENT_END = "\
} skull_config_t;\n\
#pragma pack(4)\n\
\n\
extern const skull_config_t skull_config;\n\
\n\
#endif\n\n"

SOURCE_CONTENT_START = "\
#include \"config.h\"\n\
\n\
const skull_config_t skull_config = {\n\
"

SOURCE_CONTENT_END = "};"

def load_yaml_config():
    global yaml_obj
    global config_name

    yaml_file = file(config_name, 'r')
    yaml_obj = yaml.load(yaml_file)

def _generate_header_item(name, value):
    #print "name: %s, value: %s(%s)" % (name, value, type(value))
    content = ""

    # assemble config item
    if type(value) is int:
        content += "    int %s;\n" % name
    elif type(value) is float:
        content += "    double %s;\n" % name
    elif type(value) is str:
        content += "    char %s[%d];\n" % (name, len(value) + 1)
    else:
        print "unsupported name: %s, type: %s" % (name, type(value))

    # assemble config item type
    content += "    skull_cfg_itype_t %s_type;\n\n" % name
    return content

def generate_header():
    header_file = file(header_name, 'w')
    content = ""

    # generate header
    content += HEADER_CONTENT_START

    # generate the config body
    if yaml_obj is not None:
        for name in yaml_obj:
            value = yaml_obj[name]
            content += _generate_header_item(name, value)
    else:
        # add a reserved item to avoid compile error
        content += HEADER_NO_CFG_ITEM

    # generate tailer
    content += HEADER_CONTENT_END

    # write and close the header file
    header_file.write(content)
    header_file.close()

def _generate_source_item(name, value):
    content = ""

    # assemble config item
    if type(value) is int:
        content += "    .%s = %d,\n" % (name, value)
        content += "    .%s_type = %s,\n\n" % (name, "SKULL_CFG_ITYPE_INT")
    elif type(value) is float:
        content += "    .%s = %f,\n" % (name, value)
        content += "    .%s_type = %s,\n\n" % (name, "SKULL_CFG_ITYPE_DOUBLE")
    elif type(value) is str:
        content += "    .%s = \"%s\",\n" % (name, value)
        content += "    .%s_type = %s,\n\n" % (name, "SKULL_CFG_ITYPE_STRING")
    else:
        print "unsupported name: %s, type: %s" % (name, type(value))

    return content

def generate_source():
    source_file = file(source_name, 'w')
    content = ""

    # generate header
    content += SOURCE_CONTENT_START

    # generate the config body
    if yaml_obj is not None:
        for name in yaml_obj:
            value = yaml_obj[name]
            content += _generate_source_item(name, value)
    else:
        # add a reserved item to avoid compile error
        content += SOURCE_NO_CFG_ITEM

    # generate tailer
    content += SOURCE_CONTENT_END

    # write and close the source file
    source_file.write(content)
    source_file.close()

def generate_config():
    generate_header()
    generate_source()

def usage():
    print "usage: skull-config-gen.py -c config -h header_file -o source_file"

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
        generate_config()

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        sys.exit(1)
