#!/usr/bin/python

import sys
import os
import getopt
import string
import pprint
import yaml

def load_yaml_config(config_name):
    yaml_file = open(config_name, "rb")
    return yaml_file

def show_workflows(config_name):
    yaml_file = load_yaml_config(config_name)
    yaml_obj = yaml.load(yaml_file)

    workflows = yaml_obj['workflows']
    workflow_cnt = 1

    for workflow in workflows:
        print "workflow [%d]:" % workflow_cnt
        print " - concurrent: %s" % workflow['concurrent']

        if workflow.get("port"):
            print " - port: %s" % workflow['port']

        print " - modules:"
        for module in workflow['modules']:
            print "  - %s" % module

        # increase the workflow count
        workflow_cnt += 1
        print "\n",

    print "total %d workflows" % (workflow_cnt - 1)

    yaml_file.close()

def usage():
    print "usage: skull-show_workflow.py -f yaml_file"

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        opts, args = getopt.getopt(sys.argv[1:], 'f:')

        for op, value in opts:
            if op == "-f":
                show_workflows(value)

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        sys.exit(1)
