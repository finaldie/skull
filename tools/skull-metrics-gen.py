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
config_name = ""
language = ""

####################### C language files #########################
C_HEADER_NAME = "sk_metrics.h"
C_SOURCE_NAME = "sk_metrics.c"

C_HEADER_CONTENT_START = "\
#ifndef SK_METRICS_H\n\
#define SK_METRICS_H\n\
\n\
#include <stdint.h>\n\
\n\
// base metrics type, including inc() and get() methods\n\
typedef struct sk_metrics_t {\n\
    void     (*inc)(uint32_t value);\n\
    uint32_t (*get)();\n\
} sk_metrics_t;\n\
\n"

C_HEADER_CONTENT_END = "#endif\n\n"

C_SOURCE_CONTENT_START = "\
#include <stdlib.h>\n\
#include <string.h>\n\
#include <stdio.h>\n\
\n\
#include \"api/sk_utils.h\"\n\
#include \"api/sk_mon.h\"\n\
#include \"api/sk_env.h\"\n\
#include \"api/sk_metrics.h\"\n\
\n"

C_FUNC_GLOBAL_INC_CONTENT = "static\n\
void _sk_%s_%s_inc(uint32_t value)\n\
{\n\
  sk_mon_t* mon = SK_THREAD_ENV_CORE->monitor;\n\
  sk_mon_inc(mon, \"%s\", value);\n\
}\n\n"

C_FUNC_GLOBAL_GET_CONTENT = "static\n\
uint32_t _sk_%s_%s_get()\n\
{\n\
  sk_mon_t* mon = SK_THREAD_ENV_CORE->monitor;\n\
  return sk_mon_get(mon, \"%s\");\n\
}\n\n"

C_FUNC_THREAD_INC_CONTENT = "static\n\
void _sk_%s_%s_inc(uint32_t value)\n\
{\n\
  sk_mon_t* mon = SK_THREAD_ENV_MON;\n\
  sk_mon_inc(mon, \"%s\", value);\n\
}\n\n"

C_FUNC_THREAD_GET_CONTENT = "static\n\
uint32_t _sk_%s_%s_get()\n\
{\n\
  sk_mon_t* mon = SK_THREAD_ENV_MON;\n\
  return sk_mon_get(mon, \"%s\");\n\
}\n\n"

C_API_CREATE_CONTENT_START = "sk_metrics_%s_t*\n\
sk_metrics_%s_create(const char* name)\n\
{\n\
    SK_ASSERT_MSG(name != NULL, \"%s metrics initialization failed\");\n\
\n\
    sk_metrics_%s_t* metrics = calloc(1, sizeof(*metrics));\n\
    metrics->name = strdup(name);\n\n"

C_API_CREATE_CONTENT_END = "\
\n\
    return metrics;\n\
}\n\n"

C_METRICS_INC_INIT_CONTENT = "    metrics->%s.inc = _sk_%s_%s_inc;\n"
C_METRICS_GET_INIT_CONTENT = "    metrics->%s.get = _sk_%s_%s_get;\n"

C_API_DESTROY_CONTENT = "void\n\
sk_metrics_%s_destroy(sk_metrics_%s_t* metrics)\n\
{\n\
    if (!metrics) {\n\
        return;\n\
    }\n\
\n\
    free((void*)metrics->name);\n\
    free(metrics);\n\
}\n\n"

def load_yaml_config():
    global yaml_obj
    global config_name

    yaml_file = file(config_name, 'r')
    yaml_obj = yaml.load(yaml_file)

def gen_c_header_metrics(scope_name, metrics_obj):
    # check required field
    if not metrics_obj.get("metrics"):
        print "Fatal: don't find 'metrics' field in the config, please check it again"
        sys.exit(1)

    metrics_map = metrics_obj['metrics']

    # define the type name
    metrics_type_name = "sk_metrics_" + scope_name + "_t"

    # assemble first line
    content = "/*==========================================================*/\n"
    content += "typedef struct " + metrics_type_name + " {\n"

    # assemble private datas
    content += "    // private\n"
    content += "    const char* name;\n\n"

    # assemble public data
    content += "    // public: metrics\n"

    for name in metrics_map:
        items = metrics_map[name]
        desc = items['desc']

        content += "    // " + desc + "\n"
        content += "    sk_metrics_t " + name + ";\n"

    # assemble tailer
    content += "} " + metrics_type_name + ";\n\n"

    # assemble methods
    content += "sk_metrics_%s_t* sk_metrics_%s_create(const char* name);\n" % (scope_name, scope_name)
    content += "void sk_metrics_%s_destroy(sk_metrics_%s_t*);\n" % (scope_name, scope_name)
    content += "void sk_metrics_%s_dump(sk_metrics_%s_t*);\n\n" % (scope_name, scope_name)

    return content

def generate_c_header():
    header_file = file(C_HEADER_NAME, 'w')
    content = ""

    # generate header
    content += C_HEADER_CONTENT_START

    # generate body
    for scope_name in yaml_obj:
        content += gen_c_header_metrics(scope_name, yaml_obj[scope_name])

    # generate tailer
    content += C_HEADER_CONTENT_END

    # write and close the header file
    header_file.write(content)
    header_file.close()

def gen_c_source_metrics(scope_name, metrics_obj):
    content = ""

    # 0. check required field
    if not metrics_obj.get("metrics"):
        print "Fatal: don't find 'metrics' field in the config, please check it again"
        sys.exit(1)

    metrics_map = metrics_obj['metrics']

    if not metrics_obj.get("mode"):
        print "Fatal: don't find 'mode' field in the config, please check it again"
        sys.exit(1)

    mode = metrics_obj['mode']
    if mode not in ["global", "thread"]:
        print "Fatal: 'mode' field must be 'global' or 'thread', please check it again"
        sys.exit(1)

    # 1. assemble metrics methods
    for metric_name in metrics_map:
        if mode == "global":
            # 1.1 assemble inc method
            content += C_FUNC_GLOBAL_INC_CONTENT % (scope_name, metric_name, metric_name)

            # 1.2 assemble get method
            content += C_FUNC_GLOBAL_GET_CONTENT % (scope_name, metric_name, metric_name)
        else:
            # 1.3 assemble inc method
            content += C_FUNC_THREAD_INC_CONTENT % (scope_name, metric_name, metric_name)

            # 1.4 assemble get method
            content += C_FUNC_THREAD_GET_CONTENT % (scope_name, metric_name, metric_name)

    # 2. assemble scope apis
    # 2.1 assemble create api
    content += C_API_CREATE_CONTENT_START % (scope_name, scope_name,
                                             scope_name, scope_name)

    for name in metrics_map:
        content += C_METRICS_INC_INIT_CONTENT % (name, scope_name, name)
        content += C_METRICS_GET_INIT_CONTENT % (name, scope_name, name)

    content += C_API_CREATE_CONTENT_END

    # 3. assemble destroy api
    content += C_API_DESTROY_CONTENT % (scope_name, scope_name)

    return content

def generate_c_source():
    source_file = file(C_SOURCE_NAME, 'w')
    content = ""

    # generate header
    content += C_SOURCE_CONTENT_START

    # generate body
    for scope_name in yaml_obj:
        content += gen_c_source_metrics(scope_name, yaml_obj[scope_name])

    # generate tailer

    # write and close the source file
    source_file.write(content)
    source_file.close()

def process_core():
    if language == "c":
        generate_c_header()
        generate_c_source()

def usage():
    print "usage: skull-metrics-gen.py -m core|user -c yaml_file -l language"

def process_add_workflow():
    global yaml_obj
    global config_name

    try:
        opts, args = getopt.getopt(sys.argv[5:], 'C:p:')

        workflow_concurrent = 1
        workflow_port = 1234

        for op, value in opts:
            if op == "-C":
                workflow_concurrent = int(value)
            elif op == "-p":
                workflow_port = int(value)

        # Now add these workflow_x to yaml obj and dump it
        workflow_frame = create_workflow()
        workflow_frame['concurrent'] = workflow_concurrent
        workflow_frame['port'] = workflow_port
        yaml_obj['workflows'].append(workflow_frame)

        yaml.dump(yaml_obj, file(config_name, 'w'))

    except Exception, e:
        print "Fatal: process_add: " + str(e)
        usage()
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        work_mode = None
        opts, args = getopt.getopt(sys.argv[1:], 'c:m:l:')

        for op, value in opts:
            if op == "-c":
                config_name = value
                load_yaml_config()
            elif op == "-m":
                work_mode = value
            elif op == "-l":
                language = value

        # Now run the process func according the mode
        if work_mode == "core":
            process_core()
        elif work_mode == "user":
            process_user()
        else:
            print "Fatal: Unknown work_mode: %s" % work_mode

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        sys.exit(1)
