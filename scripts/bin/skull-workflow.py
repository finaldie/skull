#!/usr/bin/python

# This is skull workflow config controlling script

import sys
import os
import getopt
import string
import pprint
import yaml

yaml_obj = None
config_name = ""

def load_yaml_config():
    global yaml_obj
    global config_name

    yaml_file = file(config_name, 'r')
    yaml_obj = yaml.load(yaml_file)

def process_show():
    workflows = yaml_obj['workflows']
    workflow_cnt = 0

    for workflow in workflows:
        print "workflow [%d]:" % workflow_cnt
        print " - concurrent: %s" % workflow['concurrent']
        print " - idl: %s" % workflow['idl']

        if workflow.get("port"):
            print " - port: %s" % workflow['port']

        print " - modules:"
        for module in workflow['modules']:
            print "  - %s" % module

        # increase the workflow count
        workflow_cnt += 1
        print "\n",

    print "total %d workflows" % (workflow_cnt)


def usage():
    print "usage: skull-workflow.py -m mode -c yaml_file ..."

def create_workflow():
    return {
    'concurrent' : 1,
    'port' : 1234,
    'modules' : []
    }

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

def process_add_module():
    global yaml_obj
    global config_name

    try:
        opts, args = getopt.getopt(sys.argv[5:], 'M:i:')

        workflow_idx = 0
        module_name = ""

        for op, value in opts:
            if op == "-M":
                module_name = value
            elif op == "-i":
                workflow_idx = int(value)

        # Now add these workflow_x to yaml obj and dump it
        workflow_frame = yaml_obj['workflows'][workflow_idx]
        workflow_frame['modules'].append(module_name)

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
        opts, args = getopt.getopt(sys.argv[1:5], 'c:m:')

        for op, value in opts:
            if op == "-c":
                config_name = value
                load_yaml_config()
            elif op == "-m":
                work_mode = value

        # Now run the process func according the mode
        if work_mode == "show":
            process_show()
        elif work_mode == "add_workflow":
            process_add_workflow()
        elif work_mode == "add_module":
            process_add_module()
        else:
            print "Fatal: Unknown work_mode: %s" % work_mode

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        sys.exit(1)
