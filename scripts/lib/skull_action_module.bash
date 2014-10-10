# This is the utility functions for skull action module:
# example: skull module --add
#
# NOTES: This is included by the main script `skull`

# Add module need 2 steps:
# 1. Add the module folder structure into project according to its module language
# 2. Change the main config
function action_module()
{
    preload_language_actions

    local skull_conf=$SKULL_PROJ_ROOT/config/skull-config.yaml

    # parse the command args
    local args=`getopt -a -o ah -l add,help \
        -n "skull_action_module.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_module_usage
        exit 1
    fi

    eval set -- "$args"

    while true; do
        case "$1" in
            -a|--add)
                shift
                action_module_add
                exit 0
                ;;
            -h|--help)
                shift
                action_module_usage
                exit 0
                ;;
            --)
                shift; break
                exit 0
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_module_usage
                exit 1
                ;;
        esac
    done
}

function action_module_usage()
{
    echo "usage: "
    echo "  skull module -a|--add"
    echo "  skull module -h|--help"
}

function preload_language_actions()
{
    # Load C Language action scripts
    . $SKULL_ROOT/lib/skull_languages/skull_actions_c.bash
}

function action_module_add()
{
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
