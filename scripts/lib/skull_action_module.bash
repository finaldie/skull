# This is the utility functions for skull action module:
# example: skull module --add
#
# NOTES: This is included by the main script `skull`

# Add module need 2 steps:
# 1. Add the module folder structure into project according to its language
# 2. Change the main config

#set -x

function action_module()
{
    # parse the command args
    local args=`getopt -a \
        -o ah \
        -l add,help,conf-gen,conf-cat,conf-edit,conf-check \
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
                action_module_usage
                exit 0
                ;;
            --conf-gen)
                shift
                _action_module_config_gen
                exit 0
                ;;
            --conf-cat)
                shift
                _action_module_config_cat
                exit 0
                ;;
            --conf-edit)
                shift
                _action_module_config_edit
                exit 0
                ;;
            --conf-check)
                shift
                _action_module_config_check
                exit 0
                ;;
            --)
                shift;
                # if there is no parameter left, show the modules directly
                if [ $# = 0 ]; then
                    _action_module_list
                fi
                break;
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_module_usage >&2
                exit 1
                ;;
        esac
    done

    # If there is no normal parameter, check whether there is module name here
    if [ $# = 0 ]; then
        exit 0;
    fi

    # There is module name exist, show the module path to user
    local module_name="$1"
    _action_module_path "$module_name"
}

function action_module_usage()
{
    echo "usage: "
    echo "  skull module -a|--add"
    echo "  skull module -h|--help"
    echo "  skull module --conf-gen"
    echo "  skull module --conf-cat"
    echo "  skull module --conf-edit"
    echo "  skull module --conf-check"
}

function _action_module_list()
{
    sk_util_module_list
}

function _action_module_path()
{
    if [ $# = 0 ]; then
        echo "Error: empty module name" >&2
        exit 1
    fi

    local module_name="$1"
    local module_path="$SKULL_PROJ_ROOT/src/modules/$module_name"
    echo "Module path: $module_path"
}

function _action_module_add()
{
    # 1. input the module name
    local module=""
    local workflow_idx=0
    local language=""
    local langs=($(sk_util_get_language_list))
    local lang_names=`echo ${langs[*]} | sed 's/ /|/g'`
    local total_workflows=`action_workflow_show | tail -1 | awk '{print $2}'`

    if ! $(sk_util_is_number $total_workflows); then
        echo "Error: Create a workflow first" >&2
        return 1
    fi

    local min_wf_idx=0
    local max_wf_idx=$((total_workflows - 1))

    # 2. get user input and verify them
    while true; do
        read -p "Module Name? " module

        if $(sk_util_check_name "$module"); then
            break;
        fi
    done

    # 3. check whether this is a existing module
    if [ -d "$SKULL_PROJ_ROOT/src/modules/$module" ]; then
        echo "Warn: Found the module [$module] has already exist, please" \
             "make sure its a valid module" >&2
    fi

    # 4. add it into a specific workflow
    local idx_range="$min_wf_idx"
    if [ $min_wf_idx -ne $max_wf_idx ]; then
        idx_range="${min_wf_idx}-${max_wf_idx}"
    fi

    while true; do
        read -p "Workflow Index? ($idx_range) " workflow_idx

        if ! $(sk_util_is_number $workflow_idx); then
            echo "Error: The input workflow index must be a number" >&2
            continue
        fi

        if [ $workflow_idx -gt $max_wf_idx ] ||
           [ $workflow_idx -lt $min_wf_idx ]; then
            echo "Error: module add failed, input the correct workflow index" >&2
        else
            break;
        fi
    done

    # NOTES: currently, we only support Cpp language
    while true; do
        read -p "Module Language? ($lang_names) " language

        # verify the language valid or not
        if $(sk_util_check_language "$language"); then
            break;
        fi
    done

    # 3. Add basic folder structure if the target module does not exist
    local IDL=`$SKULL_ROOT/bin/skull-config-utils.py -m workflow \
                -c $SKULL_CONFIG_FILE -a value -i $workflow_idx -d idl`

    action_${language}_module_add "$module" "$IDL"

    # 4. Add module into main config
    $SKULL_ROOT/bin/skull-config-utils.py -m module -c $SKULL_CONFIG_FILE \
        -a add -M $module -i $workflow_idx -l $language

    # 5. add common folder
    action_${language}_common_create

    echo "Module [$module] added successfully"
    echo ""
    echo "Note: Run 'skull module --add' to continue adding more modules"
    echo "Note: Run 'skull service --add' to create a service if needed"
}

function _action_module_config_gen()
{
    local module=$(sk_util_current_module)
    if [ -z "$module" ]; then
        echo "Error: not in a module, pwd=`pwd`" >&2
        exit 1
    fi

    local module_config=$(sk_util_current_module_config $module)
    if [ ! -f $module_config ]; then
        echo "Error: not found $module_config" >&2
        exit 1
    fi

    # now, run the config generator
    sk_util_run_module_action $module $SKULL_LANG_GEN_CONFIG $module_config
}

function _action_module_config_cat()
{
    local module=$(sk_util_current_module)
    if [ -z "$module" ]; then
        echo "Error: not in a module" >&2
        exit 1
    fi

    local module_config=$(sk_util_current_module_config $module)
    if [ ! -f $module_config ]; then
        echo "Error: not found $module_config" >&2
        exit 1
    fi

    cat $module_config
}

function _action_module_config_edit()
{
    local module=$(sk_util_current_module)
    if [ -z "$module" ]; then
        echo "Error: not in a module" >&2
        exit 1
    fi

    local module_config=$(sk_util_current_module_config $module)
    if [ ! -f $module_config ]; then
        echo "Error: not found $module_config" >&2
        exit 1
    fi

    # TODO: should load a per-user config to identify which editor will be used
    # instead of hardcode `vi` in here
    $SKULL_DEFAULT_EDITOR $module_config
}

function _action_module_config_check()
{
    echo "Error: Unimplemented!" >&2
    exit 1
}
