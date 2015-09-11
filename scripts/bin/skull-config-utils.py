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
config_path = ""

def load_yaml_config():
    global yaml_obj
    global config_name

    yaml_file = file(config_name, 'r')
    yaml_obj = yaml.load(yaml_file)

def process_show_workflow():
    workflows = yaml_obj['workflows']
    workflow_cnt = 0

    if workflows is None:
        print "no workflow"
        print "note: run 'skull workflow --add' to create a new workflow"
        return

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

def _create_workflow():
    return {
    'concurrent' : 1,
    'idl': "",
    'port' : 1234,
    'modules' : []
    }

def process_add_workflow():
    global yaml_obj
    global config_name

    try:
        # 1. update the skull config
        opts, args = getopt.getopt(sys.argv[5:], 'C:i:p:g:P:')

        workflow_concurrent = 1
        workflow_port = 1234
        workflow_idl = ""
        is_generate_idl = True
        idl_path = ""

        for op, value in opts:
            if op == "-C":
                workflow_concurrent = int(value)
            elif op == "-i":
                workflow_idl = value
            elif op == "-p":
                workflow_port = int(value)
            elif op == "-g":
                is_generate_idl = bool(value)
            elif op == "-P":
                idl_path = value

        ## 1.1 Now add these workflow_x to yaml obj and dump it
        workflow_frame = _create_workflow()
        workflow_frame['concurrent'] = workflow_concurrent
        workflow_frame['idl'] = workflow_idl
        workflow_frame['port'] = workflow_port

        ## 1.2 create if the workflows list do not exist
        if yaml_obj['workflows'] is None:
            yaml_obj['workflows'] = []

        ## 1.3 update the skull-config.yaml
        yaml_obj['workflows'].append(workflow_frame)
        yaml.dump(yaml_obj, file(config_name, 'w'))

        # 2. generate the protobuf file into config
        if is_generate_idl:
            proto_file_name = "%s/%s.proto" % (idl_path, workflow_idl)
            proto_file = file(proto_file_name, 'w')
            content = "package skull;\n\n"
            content += "message %s {\n" % workflow_idl
            content += "    required bytes data = 1;\n"
            content += "}\n"

            proto_file.write(content)
            proto_file.close()

    except Exception, e:
        print "Fatal: process_add_workflow: " + str(e)
        raise

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
        print "Fatal: process_add_module: " + str(e)
        raise

################################## Service Related #############################
def process_show_service():
    services = yaml_obj['services']
    services_cnt = 0

    if services is None:
        print "no service"
        print "note: run 'skull service --add' to create a new service"
        return

    for name in services:
        service_item = services[name]

        print "service [%s]:" % name
        print " - enable: %s" % service_item['enable']
        print " - data_mode: %s" % service_item['data_mode']
        print " - apis:"

        service_apis = service_item['apis']
        if service_apis is None:
            continue

        for api_name in service_apis:
            api = service_apis[api_name]
            print "     - %s: mode: %s" % (api_name, api['mode'])

        # increase the service count
        services_cnt += 1
        print "\n",

    print "total %d services" % (services_cnt)

def process_add_service():
    global yaml_obj
    global config_name

    try:
        opts, args = getopt.getopt(sys.argv[5:], 'N:b:d:')
        service_name = ""
        service_enable = True
        service_data_mode = ""

        for op, value in opts:
            if op == "-N":
                service_name = value
            elif op == "-b":
                service_enable = value
            elif op == "-d":
                service_data_mode = value

        # If service dict do not exist, create it
        if yaml_obj['services'] is None:
            yaml_obj['services'] = {}

        # Now add a service item into services dict
        services = yaml_obj['services']
        services[service_name] = {
            'enable' : service_enable,
            'data_mode': service_data_mode,
            'apis': {}
        }

        yaml.dump(yaml_obj, file(config_name, 'w'))

    except Exception, e:
        print "Fatal: process_add_service: " + str(e)
        raise

def process_add_service_api():
    global yaml_obj
    global config_name

    try:
        opts, args = getopt.getopt(sys.argv[5:], 'N:a:d:')
        service_name = ""
        api_name = ""
        api_access_mode = ""

        for op, value in opts:
            if op == "-N":
                service_name = value
            elif op == "-a":
                api_name = value
            elif op == "-d":
                api_access_mode = value

        if yaml_obj['services'] is None:
            yaml_obj['services'] = {}

        # Now add a service api into service dict
        services = yaml_obj['services']

        service = services[service_name]
        if service is None:
            print "Fatal: service %s section is empty" % service_name
            raise

        if service['apis'] is None:
            service['apis'] = {}

        apis = service['apis']
        apis[api_name] = {
            'mode': api_access_mode
        }

        yaml.dump(yaml_obj, file(config_name, 'w'))

    except Exception as e:
        print "Fatal: process_add_service_api: " + str(e)
        raise

################################################################################
def usage():
    print "usage:"
    print "  skull-config-utils.py -m show_workflow -c $yaml_file"
    print "  skull-config-utils.py -m add_workflow -c $yaml_file -C $concurrent -i $idl_name -p $port -g $is_gen_idl -P $idl_path"
    print "  skull-config-utils.py -m add_module -c $yaml_file -M $module_name -i $workflow_index"
    print "  skull-config-utils.py -m show_service -c $yaml_file"
    print "  skull-config-utils.py -m add_service -c $yaml_file -N $service_name -b $enable -d $data_mode"
    print "  skull-config-utils.py -m add_service_api -c $yaml_file -N $service_name -a $api_name -d $access_mode"

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        action = None
        opts, args = getopt.getopt(sys.argv[1:5], 'c:m:')

        for op, value in opts:
            if op == "-c":
                config_name = value
                config_path = os.path.dirname(config_name)
                load_yaml_config()
            elif op == "-m":
                action = value

        # Now run the process func according the mode
        if action == "show_workflow":
            process_show_workflow()
        elif action == "add_workflow":
            process_add_workflow()
        elif action == "add_module":
            process_add_module()
        elif action == "add_service":
            process_add_service()
        elif action == "show_service":
            process_show_service()
        elif action == "add_service_api":
            process_add_service_api()
        else:
            print "Fatal: Unknown action: %s" % action

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        raise
