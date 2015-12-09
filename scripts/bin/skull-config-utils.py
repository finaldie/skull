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

def _load_yaml_config(config_name):
    yaml_file = file(config_name, 'r')
    yml_obj = yaml.load(yaml_file)
    return yml_obj

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
        opts, args = getopt.getopt(sys.argv[5:], 'C:i:p:')

        workflow_concurrent = 1
        workflow_port = 1234
        workflow_idl = ""

        for op, value in opts:
            if op == "-C":
                workflow_concurrent = int(value)
            elif op == "-i":
                workflow_idl = value
            elif op == "-p":
                workflow_port = int(value)

        # 2. Now add these workflow_x to yaml obj and dump it
        workflow_frame = _create_workflow()
        workflow_frame['concurrent'] = workflow_concurrent
        workflow_frame['idl'] = workflow_idl
        workflow_frame['port'] = workflow_port

        # 3. create if the workflows list do not exist
        if yaml_obj['workflows'] is None:
            yaml_obj['workflows'] = []

        # 4. update the skull-config.yaml
        yaml_obj['workflows'].append(workflow_frame)
        yaml.dump(yaml_obj, file(config_name, 'w'))
    except Exception, e:
        print "Fatal: process_add_workflow: " + str(e)
        raise

def process_gen_workflow_idl():
    workflows = yaml_obj['workflows']

    if workflows is None:
        print "no workflow"
        print "note: run 'skull workflow --add' to create a new workflow"
        return

    idl_name = ""
    idl_path = ""

    opts, args = getopt.getopt(sys.argv[5:], 'n:p:')
    for op, value in opts:
        if op == "-n":
            idl_name = value
        elif op == "-p":
            idl_path = value

    for workflow in workflows:
        workflow_idl_name = workflow['idl']

        if idl_name != workflow_idl_name:
            continue

        proto_file_name = "%s/%s.proto" % (idl_path, idl_name)
        proto_file = file(proto_file_name, 'w')
        content = "package skull;\n\n"
        content += "message %s {\n" % idl_name
        content += "    required bytes data = 1;\n"
        content += "}\n"

        proto_file.write(content)
        proto_file.close()
        break

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

        # Check whether the module has exist in module list
        if module_name not in workflow_frame['modules']:
            workflow_frame['modules'].append(module_name)

        yaml.dump(yaml_obj, file(config_name, 'w'))

    except Exception, e:
        print "Fatal: process_add_module: " + str(e)
        raise

################################## Service Related #############################
def process_service_actions():
    try:
        opts, args = getopt.getopt(sys.argv[5:7], 'a:')

        action = ""
        service_name = ""

        for op, value in opts:
            if op == "-a":
                action = value

        if action == "exist":
            _process_service_exist()
        elif action == "add":
            _process_add_service()
        elif action == "show":
            _process_show_service()
        elif action == "add_api":
            _process_add_service_api()
        elif action == "import":
            _process_import_service()

    except Exception, e:
        print "Fatal: process_service_actions: " + str(e)
        raise

def _process_show_service():
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
            print "     - %s -> mode: %s" % (api_name, api['mode'])

        # increase the service count
        services_cnt += 1
        print "\n",

    print "total %d services" % (services_cnt)

def _process_service_exist():
    services = yaml_obj['services']

    if services is None:
        sys.exit(1)

    service_name = ""
    opts, args = getopt.getopt(sys.argv[7:], 's:')

    for op, value in opts:
        if op == "-s":
            servie_name = value

    service = services.get(service_name)
    if service is None:
        sys.exit(1)
    else:
        sys.exit(0)

def _process_add_service():
    global yaml_obj
    global config_name

    try:
        opts, args = getopt.getopt(sys.argv[7:], 's:b:d:i:')
        service_name = ""
        service_enable = True
        service_data_mode = ""
        service_yml_config = ""

        for op, value in opts:
            if op == "-s":
                service_name = value
            elif op == "-b":
                service_enable = value
            elif op == "-d":
                service_data_mode = value
            elif op == "-i":
                service_yml_config = value

        # If service dict do not exist, create it
        if yaml_obj['services'] is None:
            yaml_obj['services'] = {}

        # Now add a service item into services dict
        services = yaml_obj['services']
        service_obj = {
            'enable' : service_enable,
            'data_mode': service_data_mode,
            'apis': {}
        }

        services[service_name] = service_obj;

        yaml.dump(yaml_obj, file(config_name, 'w'))

        # update service local yml config
        service_yml_obj = {}
        service_yml_obj['service'] = service_obj
        yaml.dump(service_yml_obj, file(service_yml_config, 'w'))

    except Exception, e:
        print "Fatal: _process_add_service: " + str(e)
        raise

def _process_add_service_api():
    global yaml_obj
    global config_name

    try:
        opts, args = getopt.getopt(sys.argv[7:], 's:n:d:i:')
        service_name = ""
        api_name = ""
        api_access_mode = ""
        service_yml_config = ""

        for op, value in opts:
            if op == "-s":
                service_name = value
            elif op == "-n":
                api_name = value
            elif op == "-d":
                api_access_mode = value
            elif op == "-i":
                service_yml_config = value

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

        # Update service local yml config
        service_yml_obj = _load_yaml_config(service_yml_config)
        if service_yml_obj is None:
            service_yml_obj = {}

        if service_yml_obj['service'] is None:
            service_yml_obj['service'] = {}

        service_yml_obj['service'] = service
        yaml.dump(service_yml_obj, file(service_yml_config, 'w'))

    except Exception as e:
        print "Fatal: _process_add_service_api: " + str(e)
        raise

def _process_import_service():
    global yaml_obj
    global config_name

    try:
        opts, args = getopt.getopt(sys.argv[7:], 's:i:')
        service_name = ""
        service_yml_config = ""

        for op, value in opts:
            if op == "-s":
                service_name = value
            elif op == "-i":
                service_yml_config = value

        service_yml_obj = _load_yaml_config(service_yml_config)
        service_obj = service_yml_obj['service']

        if service_obj == None:
            print "Error: no service defined in %s" % service_yml_config
            sys.exit(1)

        # If service dict do not exist, create it
        if yaml_obj['services'] is None:
            yaml_obj['services'] = {}

        # Now add a service item into services dict
        services = yaml_obj['services']
        services[service_name] = service_obj

        yaml.dump(yaml_obj, file(config_name, 'w'))

    except Exception as e:
        print "Fatal: _process_add_service_api: " + str(e)
        raise

################################################################################
def usage():
    print "usage:"
    print "  skull-config-utils.py -m show_workflow -c $yaml_file"
    print "  skull-config-utils.py -m add_workflow -c $yaml_file -C $concurrent -i $idl_name -p $port"
    print "  skull-config-utils.py -m generate_workflow_idl -c $yaml_file -n $idl_name -p $idl_path"

    print "  skull-config-utils.py -m add_module -c $yaml_file -M $module_name -i $workflow_index"

    print "  skull-config-utils.py -m service -c $yaml_file -a show"
    print "  skull-config-utils.py -m service -c $yaml_file -a exist -s $service_name"
    print "  skull-config-utils.py -m service -c $yaml_file -a add -s $service_name -b $enabled -d $data_mode"
    print "  skull-config-utils.py -m service -c $yaml_file -a add_api -s $service_name -n $api_name -d $access_mode"

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
                yaml_obj = _load_yaml_config(config_name)
            elif op == "-m":
                action = value

        # Now run the process func according the mode
        if action == "show_workflow":
            process_show_workflow()
        elif action == "add_workflow":
            process_add_workflow()
        elif action == "add_module":
            process_add_module()
        elif action == "generate_workflow_idl":
            process_gen_workflow_idl()
        elif action == "service":
            process_service_actions()
        else:
            print "Fatal: Unknown action: %s" % action

    except Exception, e:
        print "Fatal: " + str(e)
        usage()
        raise
