# This is the utility functions for skull action module:
# example: skull module --add
#
# NOTES: This is included by the main script `skull`

# Add module need 2 steps:
# 1. Add the module folder structure into project according to its module language
# 2. Change the main config
function action_module()
{
    local skull_conf=$SKULL_PROJ_ROOT/config/skull-config.yaml

    # parse the command args
    local args=`getopt -a -o ah -l add,help \
        -n "skull_action_module.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_module_usage >&2
        exit 1
    fi

    eval set -- "$args"

    while true; do
        case "$1" in
            -a|--add)
                shift
                action_workflow_show
                echo ""
                _action_module_add
                exit 0
                ;;
            -h|--help)
                shift
                action_module_usage >&2
                exit 0
                ;;
            --)
                shift; break
                exit 0
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_module_usage >&2
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

# module name must be in [0-9a-zA-Z_]
function _check_module_name()
{
    local module_name=$1

    if [ -z "$module_name" ]; then
        echo "Error: module name must not be empty" >&2
        return 1
    fi

    local ret=`echo $module_name | grep -P "[^\w]" | wc -l`
    if [ "$ret" = "1" ]; then
        echo "Error: module name must be [0-9a-zA-Z_]" >&2
        return 1
    fi

    if [ -d $SKULL_PROJ_ROOT/components/modules/$module_name ]; then
        echo "Error: module [$module_name] has already exist, change to another name" >&2
        return 1
    fi

    return 0
}

function _action_module_add()
{
    # 1. input the module name
    local module=""
    local workflow_idx=0
    local language=""
    local langs=$(_get_language_list)
    local lang_names=`echo ${langs[*]} | sed 's/ /|/g'`
    local total_workflows=`action_workflow_show | tail -1 | awk '{print $2}'`

    # 2. get user input and verify them
    while true; do
        read -p "module name? " module

        if $(_check_module_name $module); then
            break;
        fi
    done

    while true; do
        read -p "which workflow you want add it to? " workflow_idx

        if [ "$workflow_idx" -ge "$total_workflows" ] ||
           [ "$workflow_idx" -lt "0" ]; then
            echo "Fatal: module add failed, input the correct workflow index" >&2
        else
            break;
        fi
    done

    while true; do
        read -p "which language the module belongs to?($lang_names) " language

        # verify the language valid or not
        if $(_check_language $langs $language); then
            break;
        fi
    done

    # 3. Add basic folder structure
    # NOTES: currently, we only support C language
    action_${language}_add $module

    # 4. Add module into main config
    $SKULL_ROOT/bin/skull-workflow.py -m add_module -c $skull_conf -M $module -i $workflow_idx
    echo "module [$module] added successfully"
}
