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

# static
CONFIG_NAME = "/config/metrics.yaml"

####################### C language files #########################
HEADER_NAME = "skull_metrics.h"
SOURCE_NAME = "skull_metrics.c"

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
#include <stdint.h>\n\
\n\
// base metrics type, including inc() and get() methods\n\
typedef struct skull_metrics_t {\n\
    void     (*inc)(uint32_t value);\n\
    uint32_t (*get)();\n\
} skull_metrics_t;\n\
\n\
typedef struct skull_metrics_dynamic_t {\n\
    void     (*inc)(const char* name, uint32_t value);\n\
    uint32_t (*get)(const char* name);\n\
} skull_metrics_dynamic_t;\n\
\n"

HEADER_CONTENT_END = "\
 #ifdef __cplusplus\n\
extern \"C\" {\n\
#endif\n\
\n\
#endif\n\n"

########################## Source Part ###########################
SOURCE_CONTENT_START = "\
#include <stdlib.h>\n\
#include <string.h>\n\
#include <stdio.h>\n\
\n\
#include \"skull_metrics.h\"\n\
\n"

FUNC_INC_CONTENT = "static\n\
void _skull_%s_%s_inc(uint32_t value)\n\
{\n\
    sk_mon_t* mon = sk_thread_env()->core->mon;\n\
    sk_mon_inc(mon, \"%s\", value);\n\
}\n\n"

FUNC_GET_CONTENT = "static\n\
uint32_t _skull_%s_%s_get()\n\
{\n\
    sk_mon_t* mon = sk_thread_env()->core->mon;\n\
    return sk_mon_get(mon, \"%s\");\n\
}\n\n"

# dynamic metrics
## global dynamic metrics
FUNC_DYN_INC_CONTENT = "static\n\
void _skull_dynamic_inc(const char* name, uint32_t value)\n\
{\n\
    sk_mon_t* mon = sk_thread_env()->core->mon;\n\
    sk_mon_inc(mon, name, value);\n\
}\n\n"

FUNC_DYN_GET_CONTENT = "static\n\
uint32_t _skull_dynamic_get(const char* name)\n\
{\n\
    sk_mon_t* mon = sk_thread_env()->core->mon;\n\
    return sk_mon_get(mon, name);\n\
}\n\n"

API_CREATE_CONTENT_START = "skull_metrics_%s_t*\n\
skull_metrics_%s_create(const char* name)\n\
{\n\
    SK_ASSERT_MSG(name != NULL, \"%s metrics initialization failed\");\n\
\n\
    skull_metrics_%s_t* metrics = calloc(1, sizeof(*metrics));\n\
    metrics->name = strdup(name);\n\n"

API_CREATE_CONTENT_END = "\
\n\
    return metrics;\n\
}\n\n"

METRICS_INC_INIT_CONTENT = "    metrics->%s.inc = _skull_%s_%s_inc;\n"
METRICS_GET_INIT_CONTENT = "    metrics->%s.get = _skull_%s_%s_get;\n"

METRICS_DYN_INC_INIT_CONTENT = "    metrics->dynamic.inc = _skull_dynamic_inc;\n"
METRICS_DYN_GET_INIT_CONTENT = "    metrics->dynamic.get = _skull_dynamic_get;\n"

API_DESTROY_CONTENT = "void\n\
skull_metrics_%s_destroy(skull_metrics_%s_t* metrics)\n\
{\n\
    if (!metrics) {\n\
        return;\n\
    }\n\
\n\
    free((void*)metrics->name);\n\
    free(metrics);\n\
}\n\n"

############################## Internal APIs ############################
def load_yaml_config():
    global yaml_obj
    global CONFIG_NAME

    yaml_file = file(topdir + CONFIG_NAME, 'r')
    yaml_obj = yaml.load(yaml_file)

def gen_c_header_metrics(scope_name, metrics_map):
    # define the type name
    metrics_type_name = "skull_metrics_" + scope_name + "_t"

    # assemble first line
    content = "/*==========================================================*/\n"
    content += "typedef struct " + metrics_type_name + " {\n"

    # assemble private datas
    content += "    // private\n"
    content += "    const char* name;\n\n"

    # assemble public data
    content += "    // public metrics:\n"
    content += "    // dynamic metrics handler\n"
    content += "    skull_metrics_dynamic_t dynamic;\n"

    for name in metrics_map:
        items = metrics_map[name]
        desc = items['desc']

        content += "    // " + desc + "\n"
        content += "    skull_metrics_t " + name + ";\n"

    # assemble tailer
    content += "} " + metrics_type_name + ";\n\n"

    # assemble methods
    content += "skull_metrics_%s_t* skull_metrics_%s_create(const char* name);\n" % (scope_name, scope_name)
    content += "void skull_metrics_%s_destroy(skull_metrics_%s_t*);\n" % (scope_name, scope_name)
    content += "void skull_metrics_%s_dump(skull_metrics_%s_t*);\n\n" % (scope_name, scope_name)

    return content

def generate_c_header():
    header_file = file(HEADER_NAME, 'w')
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

def gen_c_source_metrics(scope_name, metrics_map):
    content = ""

    # 1. assemble metrics methods
    for metric_name in metrics_map:
        # 1.1 assemble inc method
        content += FUNC_INC_CONTENT % (scope_name, metric_name, metric_name)

        # 1.2 assemble get method
        content += FUNC_GET_CONTENT % (scope_name, metric_name, metric_name)

    # 2. assemble scope apis
    # 2.1 assemble create api
    content += API_CREATE_CONTENT_START % (scope_name, scope_name,
                                             scope_name, scope_name)
    # 2.2 assemble dynamic metrics api
    content += METRICS_DYN_INC_INIT_CONTENT
    content += METRICS_DYN_GET_INIT_CONTENT

    # 2.3 assemble static metris api
    for name in metrics_map:
        content += METRICS_INC_INIT_CONTENT % (name, scope_name, name)
        content += METRICS_GET_INIT_CONTENT % (name, scope_name, name)

    content += API_CREATE_CONTENT_END

    # 3. assemble destroy api
    content += API_DESTROY_CONTENT % (scope_name, scope_name)

    return content

def generate_c_source():
    source_file = file(SOURCE_NAME, 'w')
    content = ""

    # generate header
    content += SOURCE_CONTENT_START

    # generate dynamic metrcis implementations
    content += FUNC_DYN_INC_CONTENT
    content += FUNC_DYN_GET_CONTENT

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
    print "usage: skull-metrics-gen.py -p topdir"

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'p:')

        for op, value in opts:
            if op == "-p":
                topdir = value
                load_yaml_config()

        # Now run the process func according the mode
        process_core()

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        sys.exit(1)
