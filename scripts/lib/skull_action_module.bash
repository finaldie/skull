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
    local skull_conf=$SKULL_PROJ_ROOT/config/skull-config.yaml

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
    _module_list
}

function _action_module_path()
{
    if [ $# = 0 ]; then
        echo "Error: empty module name" >&2
        exit 1
    fi

    local module_name="$1"
    local module_location="$SKULL_PROJ_ROOT/src/modules/$module_name"
    echo "module location: $module_location"
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

        if $(_check_name "$module"); then
            break;
        fi
    done

    # 3. check whether this is a existing module
    if [ -d "$SKULL_PROJ_ROOT/src/modules/$module" ]; then
        echo "Warn: Found the module [$module] has already exist, please" \
             "make sure its a valid module" >&2
    fi

    # 4. add it into a specific workflow
    while true; do
        read -p "which workflow you want add it to? " workflow_idx

        if [ "$workflow_idx" -ge "$total_workflows" ] ||
           [ "$workflow_idx" -lt "0" ]; then
            echo "Fatal: module add failed, input the correct workflow index" >&2
        else
            break;
        fi
    done

    # NOTES: currently, we only support C language
    while true; do
        read -p "which language the module belongs to? ($lang_names) " language

        # verify the language valid or not
        if $(_check_language $langs "$language"); then
            break;
        fi
    done

    # 3. Add basic folder structure if the target module does not exist
    action_${language}_module_add $module

    # 4. Add module into main config
    $SKULL_ROOT/bin/skull-config-utils.py -m module -c $skull_conf \
        -a add -M $module -i $workflow_idx

    # 5. add common folder
    action_${language}_common_create

    echo "module [$module] added successfully"
}

function _action_module_config_gen()
{
    local module=$(_current_module)
    if [ -z "$module" ]; then
        echo "Error: not in a module" >&2
        exit 1
    fi

    local module_config=$(_current_module_config $module)
    if [ ! -f $module_config ]; then
        echo "Error: not found $module_config" >&2
        exit 1
    fi

    # now, run the config generator
    _run_module_action $SKULL_LANG_GEN_CONFIG $module_config
}

function _action_module_config_cat()
{
    local module=$(_current_module)
    if [ -z "$module" ]; then
        echo "Error: not in a module" >&2
        exit 1
    fi

    local module_config=$(_current_module_config $module)
    if [ ! -f $module_config ]; then
        echo "Error: not found $module_config" >&2
        exit 1
    fi

    cat $module_config
}

function _action_module_config_edit()
{
    local module=$(_current_module)
    if [ -z "$module" ]; then
        echo "Error: not in a module" >&2
        exit 1
    fi

    local module_config=$(_current_module_config $module)
    if [ ! -f $module_config ]; then
        echo "Error: not found $module_config" >&2
        exit 1
    fi

    # TODO: should load a per-user config to identify which editor will be used
    # instead of hardcode `vi` in here
    vim $module_config
}

function _action_module_config_check()
{
    echo "Error: Unimplemented!" >&2
    exit 1
}
