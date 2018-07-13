#!/usr/bin/env python3

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
config_name = ""

METRICS_MODE = ["global", "thread"]

####################### C language files #########################
HEADER_NAME = "sk_metrics.h"
SOURCE_NAME = "sk_metrics.c"

####################### STATIC TEMPALTES #########################

########################## Header Part ###########################
HEADER_CONTENT_START = "\
#ifndef SK_METRICS_H\n\
#define SK_METRICS_H\n\
\n\
// base metrics type, including inc() and get() methods\n\
typedef struct sk_metrics_t {\n\
    void   (*inc)(double value);\n\
    double (*get)();\n\
} sk_metrics_t;\n\
\n"

HEADER_CONTENT_END = "#endif\n\n"

########################## Source Part ###########################
SOURCE_CONTENT_START = "\
#include <stdlib.h>\n\
#include <string.h>\n\
#include <stdio.h>\n\
\n\
#include \"api/sk_utils.h\"\n\
#include \"api/sk_mon.h\"\n\
#include \"api/sk_env.h\"\n\
#include \"api/sk_metrics.h\"\n\
\n"

FUNC_GLOBAL_INC_CONTENT = "static\n\
void _sk_%s_%s_inc(double value)\n\
{\n\
    sk_mon_t* mon = SK_ENV_CORE->mon;\n\
    sk_mon_inc(mon, \"skull.core.g.%s.%s\", value);\n\
}\n\n"

FUNC_GLOBAL_GET_CONTENT = "static\n\
double _sk_%s_%s_get()\n\
{\n\
    sk_mon_t* mon = SK_ENV_CORE->mon;\n\
    return sk_mon_get(mon, \"skull.core.g.%s.%s\");\n\
}\n\n"

FUNC_THREAD_INC_CONTENT = "static\n\
void _sk_%s_%s_inc(double value)\n\
{\n\
    sk_mon_t* mon = SK_ENV_MON;\n\
    char name[256] = {0};\n\
    snprintf(name, 256, \"skull.core.t.%s.%%s.%s\",\n\
             SK_ENV->name);\n\
    sk_mon_inc(mon, name, value);\n\
}\n\n"

FUNC_THREAD_GET_CONTENT = "static\n\
double _sk_%s_%s_get()\n\
{\n\
    sk_mon_t* mon = SK_ENV_MON;\n\
    char name[256] = {0};\n\
    snprintf(name, 256, \"skull.core.t.%s.%%s.%s\",\n\
             SK_ENV->name);\n\
    return sk_mon_get(mon, name);\n\
}\n\n"

METRICS_INIT_CONTENT = "    .%s = { .inc = _sk_%s_%s_inc, .get = _sk_%s_%s_get},\n"

############################## Internal APIs ############################
def load_yaml_config():
    global yaml_obj
    global config_name

    yaml_file = open(config_name, 'r')
    yaml_obj = yaml.load(yaml_file)

def gen_c_header_metrics(scope_name, metrics_obj):
    # check required field
    if not metrics_obj.get("metrics"):
        print ("Fatal: don't find 'metrics' field in the config, please check it again")
        sys.exit(1)

    metrics_map = metrics_obj['metrics']

    # define the type name
    metrics_type_name = "sk_metrics_" + scope_name + "_t"

    # assemble first line
    content = "/*==========================================================*/\n"
    content += "typedef struct " + metrics_type_name + " {\n"

    for name in metrics_map:
        items = metrics_map[name]
        desc = items['desc']

        content += "    // " + desc + "\n"
        content += "    sk_metrics_t " + name + ";\n"

    # assemble tailer
    content += "} " + metrics_type_name + ";\n\n"

    # add user handler
    content += "extern sk_metrics_%s_t sk_metrics_%s;\n\n" % (scope_name, scope_name)

    return content

def generate_c_header():
    header_file = open(HEADER_NAME, 'w')
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
    global METRICS_MODE
    content = ""

    # 0. check required field
    if not metrics_obj.get("metrics"):
        print ("Fatal: don't find 'metrics' field in the config, please check it again")
        sys.exit(1)

    metrics_map = metrics_obj['metrics']

    if not metrics_obj.get("mode"):
        print ("Fatal: don't find 'mode' field in the config, please check it again")
        sys.exit(1)

    mode = metrics_obj['mode']
    if mode not in METRICS_MODE:
        print ("Fatal: 'mode' field must be 'global' or 'thread', please check it again")
        sys.exit(1)

    # 1. assemble metrics methods
    for metric_name in metrics_map:
        if mode == "global":
            # 1.1 assemble inc method
            content += FUNC_GLOBAL_INC_CONTENT % (scope_name, metric_name, scope_name, metric_name)

            # 1.2 assemble get method
            content += FUNC_GLOBAL_GET_CONTENT % (scope_name, metric_name, scope_name, metric_name)
        else:
            # 1.3 assemble inc method
            content += FUNC_THREAD_INC_CONTENT % (scope_name, metric_name, scope_name, metric_name)

            # 1.4 assemble get method
            content += FUNC_THREAD_GET_CONTENT % (scope_name, metric_name, scope_name, metric_name)

    # 2. assemble metrics
    # 2.1 assemble create api header
    content += "sk_metrics_%s_t sk_metrics_%s = {\n" % (scope_name, scope_name)

    # 2.2 assemble metrics handler
    for name in metrics_map:
        content += METRICS_INIT_CONTENT % (name, scope_name, name, scope_name, name)

    # 2.2 assemble api tailer
    content += "};\n\n"

    return content

def generate_c_source():
    source_file = open(SOURCE_NAME, 'w')
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
    print ("usage: skull-metrics-gen.py -c yaml_file")

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'c:')

        for op, value in opts:
            if op == "-c":
                config_name = value
                load_yaml_config()

        # Now run the process func according the mode
        process_core()

    except Exception as e:
        print ("Fatal: " + str(e))
        usage()
        sys.exit(1)
