# This is a utility functions for skull actions including:
#  - create
#  - show
#  - add
#  - rm
#  ...
# NOTES: This is included by the main script `skull`

function skull_create()
{
    local workspace=$1
    # build the basic workspace folder structure
    mkdir -p $workspace
    mkdir -p $workspace/.skull
    mkdir -p $workspace/components/modules
    mkdir -p $workspace/components/common
    mkdir -p $workspace/scripts
    mkdir -p $workspace/tests
    mkdir -p $workspace/config

    # copy templates to the target workspace
    cp $SKULL_ROOT/share/skull/Makefile $workspace/Makefile
    cp $SKULL_ROOT/share/skull/skull-start.sh $workspace/scripts/skull-start.sh
    cp $SKULL_ROOT/share/skull/skull-stop.sh $workspace/scripts/skull-stop.sh
    cp $SKULL_ROOT/etc/skull/skull-config.yaml $workspace/config/skull-config.yaml
}

function action_create()
{
    if [ $# = 0 ]; then
        echo "Error: missing project name"
        print_usage
        exit 1
    fi

    local workspace=$1
    echo "create skull workspace..."
    if [ -d $workspace ] && [ -d $workspace/.skull ]; then
        echo "Notice: The workspace [$workspace] is a skull project, " \
             "give up to build it"
        exit 1
    fi

    skull_create $workspace
    echo "create skull workspace done"
}

function action_show()
{
    local skull_conf=$SKULL_PROJ_ROOT/config/skull-config.yaml

    $SKULL_ROOT/bin/skull-workflow.py -m show -c $skull_conf
}

function action_add_workflow()
{
    local skull_conf=$SKULL_PROJ_ROOT/config/skull-config.yaml
    local concurrent=1
    local port=1234

    local yn_concurrent=true
    local yn_port=true

    # set the concurrent
    read -p "Whether allow concurrent? (y/n)" yn_concurrent
    if [ ! $yn_concurrent = "y" ]; then
        concurrent=0
    fi

    # set the port
    read -p "Need listen on a port? (y/n)" yn_port
    if [ $yn_port = "y" ]; then
        read -p "Input the port you want(1025-65535): " port
    fi

    $SKULL_ROOT/bin/skull-workflow.py -m add_workflow -c $skull_conf -C $concurrent -p $port
}

# Add module need 2 steps:
# 1. Add the module folder structure into project according to its module language
# 2. Change the main config
function action_add_module()
{
    local skull_conf=$SKULL_PROJ_ROOT/config/skull-config.yaml

    # 1. input the module name
    local module=""
    local workflow_idx=0
    local language=""

    read -p "module name? " module
    read -p "which workflow you want add it to? " workflow_idx
    read -p "which language the module belongs to? " language

    # 1. Add basic folder structure
    # NOTES: currently, we only support C language
    action_${language}_add $module

    # 2. Add module into main config
    $SKULL_ROOT/bin/skull-workflow.py -m add_module -c $skull_conf -M $module -i $workflow_idx
    echo "module [$module] added successfully"
}

function action_start()
{
    local start_mode=$1
    local skull_conf=$SKULL_PROJ_ROOT/config/skull-config.yaml
    local deploy_dir=

    if [ $start_mode = "dev" ]; then
        deploy_dir=$SKULL_PROJ_ROOT/run
    elif [ $start_mode = "prod" ]; then
        echo "Fatal: Un-implemented mode"
        exit 1
    else
        echo "Fatal: Unknown start_mode $start_mode"
        exit 1
    fi

    # Fork a process to do the deployment, since we will change the working
    # dir, this will be easily to handle the cleanup job
    (
        if [ $start_mode = "dev" ]; then
            action_deploy dev

            cd $deploy_dir
            skull-engine -c $skull_conf
        elif [ $start_mode = "prod" ]; then
            echo "Fatal: Un-implemented mode"
            exit 1
        else
            echo "Fatal: Unknown start_mode $start_mode"
            exit 1
        fi
    )
}

function action_deploy()
{
    local deploy_mode=$1

    if [ $deploy_mode = "dev" ]; then
        _action_deploy_dev
    elif [ $deploy_mode = "prod" ]; then
        _action_deploy_prod
    else
        echo "Fatal: Unknown deploy mode \"$deploy_mode\""
        exit 1
    fi
}

function _action_deploy_dev()
{
    # Fork a process to do the deployment, since we will change the working
    # dir, this will be easily to handle the cleanup job
    (
        cd $SKULL_PROJ_ROOT
        make deploy DEPLOY_DIR=./run
    )
}

function _action_deploy_prod()
{
    echo "Fatal: Un-implemented mode"
    exit 1
}
