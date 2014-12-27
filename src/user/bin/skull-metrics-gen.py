#!/usr/bin/python

# This is skull metrics generator and used only for the skull engine itself.
# It will generate the C header and source files

import sys
import os
import getopt
import string
import pprint
import yaml

# global variables
yaml_obj = None
topdir = ""
config_name = ""
header_name = ""
source_name = ""

METRICS_TYPE = ["static", "dynamic"]

####################### STATIC TEMPALTES #########################
########################## Header Part ###########################
HEADER_CONTENT_START = "\
#ifndef SKULL_METRICS_H\n\
#define SKULL_METRICS_H\n\
\n\
#ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\
\n\
// base metrics type, including inc() and get() methods\n\
typedef struct skull_metrics_t {\n\
    void   (*inc)(double value);\n\
    double (*get)();\n\
} skull_metrics_t;\n\
\n\
typedef struct skull_metrics_dynamic_t {\n\
    void   (*inc)(double value, const char* fmt, ...);\n\
    double (*get)(const char* fmt, ...);\n\
} skull_metrics_dynamic_t;\n\
\n"

HEADER_CONTENT_END = "\
#ifdef __cplusplus\n\
}\n\
#endif\n\
\n\
#endif\n\n"

########################## Source Part ###########################
SOURCE_CONTENT_START = "\
#include <stdlib.h>\n\
#include <string.h>\n\
#include <stdio.h>\n\
#include <stdarg.h>\n\
\n\
#include <skull/metrics_utils.h>\n\
#include \"skull_metrics.h\"\n\
\n"

FUNC_INC_CONTENT = "static\n\
void _skull_%s_%s_inc(double value)\n\
{\n\
    skull_metric_inc(\"skull.user.s.%s.%s\", value);\n\
}\n\n"

FUNC_GET_CONTENT = "static\n\
double _skull_%s_%s_get()\n\
{\n\
    return skull_metric_get(\"skull.user.s.%s.%s\");\n\
}\n\n"

# dynamic metrics
## global dynamic metrics
FUNC_DYN_INC_CONTENT = "static\n\
void _skull_%s_%s_dynamic_inc(double value, const char* fmt, ...)\n\
{\n\
    char full_name[256] = {0};\n\
    int hlen = snprintf(full_name, 256, \"skull.user.d.%s.%s.\");\n\
    va_list ap;\n\
    va_start(ap, fmt);\n\
    vsnprintf(full_name + hlen, 256 - hlen, fmt, ap);\n\
    skull_metric_inc(full_name, value);\n\
    va_end(ap);\n\
}\n\n"

FUNC_DYN_GET_CONTENT = "static\n\
double _skull_%s_%s_dynamic_get(const char* fmt, ...)\n\
{\n\
    char full_name[256] = {0};\n\
    int hlen = snprintf(full_name, 256, \"skull.user.d.%s.%s.\");\n\
    va_list ap;\n\
    va_start(ap, fmt);\n\
    vsnprintf(full_name + hlen, 256 - hlen, fmt, ap);\n\
    va_end(ap);\n\
    return skull_metric_get(full_name);\n\
}\n\n"

METRICS_INIT_CONTENT = "    .%s = { .inc = _skull_%s_%s_inc, .get =  _skull_%s_%s_get},\n"
METRICS_DYN_INIT_CONTENT = "    .%s = { .inc = _skull_%s_%s_dynamic_inc, .get =  _skull_%s_%s_dynamic_get},\n"

############################## Internal APIs ############################
def load_yaml_config():
    global yaml_obj
    global config_name

    yaml_file = file(config_name, 'r')
    yaml_obj = yaml.load(yaml_file)

def gen_c_header_metrics(scope_name, metrics_obj):
    global METRICS_TYPE

    if not metrics_obj.get("type"):
        print "Fatal: don't find 'type' field in the config, please check it again"
        sys.exit(1)

    type = metrics_obj['type']
    if type not in METRICS_TYPE:
        print "Fatal: 'type' field must be 'static' or 'dynamic', please check it again"
        sys.exit(1)

    if not metrics_obj.get("metrics"):
        print "Fatal: don't find 'metrics' field in the config, please check it again"
        sys.exit(1)

    metrics_map = metrics_obj['metrics']

    # define the type name
    metrics_type_name = "skull_metrics_%s_t" % scope_name

    # assemble first line
    content = "/*==========================================================*/\n"
    content += "typedef struct " + metrics_type_name + " {\n"

    for name in metrics_map:
        items = metrics_map[name]
        desc = items['desc']

        content += "    // " + desc + "\n"

        if type == "static":
            content += "    skull_metrics_t " + name + ";\n"
        else:
            content += "    skull_metrics_dynamic_t " + name + ";\n"

    # assemble tailer
    content += "} " + metrics_type_name + ";\n\n"

    # add a user handler
    content += "extern skull_metrics_%s_t skull_metrics_%s;\n" % (scope_name, scope_name)

    return content

def generate_c_header():
    header_file = file(header_name, 'w')
    content = ""

    # generate header
    content += HEADER_CONTENT_START

    # generate body
    for scope_name in yaml_obj:
        content += gen_c_header_metrics(scope_name, yaml_obj[scope_name])

    # generate tailer
    content += HEADER_CONTENT_END

    # write and close the header file
    header_file.write(content)
    header_file.close()

def gen_c_source_metrics(scope_name, metrics_obj):
    global METRICS_TYPE

    if not metrics_obj.get("type"):
        print "Fatal: don't find 'type' field in the config, please check it again"
        sys.exit(1)

    type = metrics_obj['type']
    if type not in METRICS_TYPE:
        print "Fatal: 'type' field must be 'static' or 'dynamic', please check it again"
        sys.exit(1)

    if not metrics_obj.get("metrics"):
        print "Fatal: don't find 'metrics' field in the config, please check it again"
        sys.exit(1)

    metrics_map = metrics_obj['metrics']

    content = "// ==========================================================\n"

    # 1. assemble metrics methods
    for name in metrics_map:
        if type == "static":
            content += FUNC_INC_CONTENT % (scope_name, name, scope_name, name)
            content += FUNC_GET_CONTENT % (scope_name, name, scope_name, name)
        else:
            content += FUNC_DYN_INC_CONTENT % (scope_name, name, scope_name, name)
            content += FUNC_DYN_GET_CONTENT % (scope_name, name, scope_name, name)

    # 2. assemble metrics
    # 2.1 assemble the metrics header
    content += "skull_metrics_%s_t skull_metrics_%s = {\n" % (scope_name, scope_name)

    # 2.2 assemble static metris api
    for name in metrics_map:
        if type == "static":
            content += METRICS_INIT_CONTENT % (name, scope_name, name, scope_name, name)
        else:
            content += METRICS_DYN_INIT_CONTENT % (name, scope_name, name, scope_name, name)

    # 2.3 end of the metrics
    content += "};\n\n"

    return content

def generate_c_source():
    source_file = file(source_name, 'w')
    content = ""

    # generate header
    content += SOURCE_CONTENT_START

    # generate body
    for scope_name in yaml_obj:
        content += gen_c_source_metrics(scope_name, yaml_obj[scope_name])

    # generate tailer

    # write and close the source file
    source_file.write(content)
    source_file.close()

def process_core():
    generate_c_header()
    generate_c_source()

def usage():
    print "usage: skull-metrics-gen.py -p topdir -h header_file -o source_file"

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
        process_core()

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        sys.exit(1)
