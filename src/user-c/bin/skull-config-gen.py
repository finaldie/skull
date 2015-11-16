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
    char* _nothing;\n\
"

SOURCE_NO_CFG_ITEM = "\
    new_config->_nothing = \"there is no config item\";\n\
"

HEADER_CONTENT_START = "\
#ifndef SKULL_STATIC_CONFIG_H\n\
#define SKULL_STATIC_CONFIG_H\n\
\n\
#include <skull/config.h>\n\
\n\
#pragma pack(4)\n\
typedef struct skull_static_config_t {\
\n"

HEADER_CONTENT_END = "\
} skull_static_config_t;\n\
#pragma pack(4)\n\
\n\
// return the const static pointer to the static config obj\n\
const skull_static_config_t* skull_static_config();\n\
\n\
/**\n\
 * Convert raw skull config obj to static config obj and\n\
 * it will free the old static config obj if exist\n\
 *\n\
 * @note This is not a thread-safe api, it only can be called\n\
 *             multiple times in UT environment\n\
 * @return none, but if skull_config_getxxx api failed, it will\n\
 *               print message to stderr and exit with '1'.\n\
 */\n\
void skull_static_config_convert(const skull_config_t* config);\n\
\n\
void skull_static_config_destroy();\n\
\n\
#endif\n\n"

SOURCE_CONTENT_START = "\
#include <stdlib.h>\n\
#include \"config.h\"\n\
\n\
"

SOURCE_CONTENT_STATIC_CONFIG = "\
static skull_static_config_t* g_static_config = NULL;\n\
\n\
// Internal APIs\n\
static\n\
skull_static_config_t* _skull_static_config_create()\n\
{\n\
    return calloc(1, sizeof(skull_static_config_t));\n\
}\n\
\n\
static\n\
void _skull_static_config_destroy(skull_static_config_t* config)\n\
{\n\
    if (!config) {\n\
        return;\n\
    }\n\
\n\
    free(config);\n\
}\n\
\n\
static\n\
void _skull_static_config_reset(skull_static_config_t* config)\n\
{\n\
   skull_static_config_t* old = g_static_config;\n\
   g_static_config = config;\n\
   _skull_static_config_destroy(old);\n\
}\n\
\n\
// Public APIs\n\
const skull_static_config_t* skull_static_config()\n\
{\n\
    return g_static_config;\n\
}\n\
\n\
void skull_static_config_destroy()\n\
{\n\
    _skull_static_config_destroy(g_static_config);\n\
}\n\
"

SOURCE_CONTENT_CONVERTOR_START = "\
void skull_static_config_convert(const skull_config_t* config)\n\
{\n\
    skull_static_config_t* new_config = _skull_static_config_create();\n\
"

SOURCE_CONTENT_CONVERTOR_END = "\
\n\
    _skull_static_config_reset(new_config);\n\
}\n\
"

SOURCE_CONTENT_END = ""

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
        content += "    char* %s;\n" % name
    else:
        print "unsupported name: %s, type: %s" % (name, type(value))

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

def _generate_convertor_source_item(name, value):
    content = ""

    # assemble config item
    if type(value) is int:
        content += "    new_config->%s = skull_config_getint(config, \"%s\", 0);\n" % (name, name)
    elif type(value) is float:
        content += "    new_config->%s = skull_config_getdouble(config, \"%s\", 0.0f);\n" % (name, name)
    elif type(value) is str:
        content += "    new_config->%s = (char*)skull_config_getstring(config, \"%s\");\n" % (name, name)
    else:
        print "unsupported name: %s, type: %s" % (name, type(value))

    return content

def generate_source():
    source_file = file(source_name, 'w')
    content = ""

    # generate header
    content += SOURCE_CONTENT_START
    content += SOURCE_CONTENT_STATIC_CONFIG

    # generate the convertor logic
    content += SOURCE_CONTENT_CONVERTOR_START
    if yaml_obj is not None:
        for name in yaml_obj:
            value = yaml_obj[name]
            content += _generate_convertor_source_item(name, value)
    else:
        # add a reserved item to avoid compile error
        content += SOURCE_NO_CFG_ITEM

    content += SOURCE_CONTENT_CONVERTOR_END
    # generate tailer
    content += SOURCE_CONTENT_END

    # write and close the source file
    source_file.write(content)
    source_file.close()

def generate_config():
    generate_header()
    generate_source()

def usage():
    print "usage: skull-config-gen.py -c config -h header_file -s source_file"

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
