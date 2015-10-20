# This is the utility functions for skull action module:
# example: skull module --add
#
# NOTES: This is included by the main script `skull`

# Add a service need 2 steps:
# 1. Add the service folder structure into project according to its language
# 2. Change the main config

function action_service()
{
    # parse the command args
    local args=`getopt -a \
        -o ash \
        -l add,list,help,conf-gen,conf-cat,conf-edit,conf-check,api-list,api-add,api-cat,api-edit,api-check,api-gen \
        -n "skull_action_service.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_service_usage >&2
        exit 1
    fi

    eval set -- "$args"

    while true; do
        case "$1" in
            -a|--add)
                shift
                _action_service_add
                exit 0
                ;;
            -l|--list)
                shift
                action_service_show
                exit 0
                ;;
            -h|--help)
                shift
                action_service_usage >&2
                exit 0
                ;;
            --conf-gen)
                shift
                _action_service_config_gen
                exit 0
                ;;
            --conf-cat)
                shift
                _action_service_config_cat
                exit 0
                ;;
            --conf-edit)
                shift
                _action_service_config_edit
                exit 0
                ;;
            --conf-check)
                shift
                _action_service_config_check
                exit 0
                ;;
            --api-list)
                shift
                _action_service_api_list
                exit 0
                ;;
            --api-add)
                shift 2
                _action_service_api_add
                exit 0
                ;;
            --api-cat)
                shift 2
                _action_service_api_cat $@
                exit 0
                ;;
            --api-edit)
                shift 2
                _action_service_api_edit $@
                exit 0
                ;;
            --api-check)
                shift 2
                _action_service_api_check
                exit 0
                ;;
            --api-gen)
                shift 2
                _action_service_api_gen
                exit 0
                ;;
            --)
                shift; break
                exit 0
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_service_usage >&2
                exit 1
                ;;
        esac
    done
}

function action_service_usage()
{
    echo "usage: "
    echo "  skull service -a|--add"
    echo "  skull service -l|--list"
    echo "  skull service -h|--help"
    echo "  skull service --conf-gen"
    echo "  skull service --conf-cat"
    echo "  skull service --conf-edit"
    echo "  skull service --conf-check"

    echo "  skull service --api-list"
    echo "  skull service --api-add"
    echo "  skull service --api-cat"
    echo "  skull service --api-edit"
    echo "  skull service --api-gen"
    echo "  skull service --api-check"
}

function action_service_show()
{
    $SKULL_ROOT/bin/skull-config-utils.py -m show_service -c $SKULL_CONFIG_FILE
}

function __validate_data_mode()
{
    local data_mode=$1
    local valid_data_modes=('exclusive' 'rw-pr' 'rw-pw')

    for mode in ${valid_data_modes[*]}; do
        if [ "$mode" = "$data_mode" ]; then
            return 0
        fi
    done

    return 1
}

function _action_service_add()
{
    # 1. input the module name
    local service=""
    local language=""
    local data_mode=""
    local langs=$(_get_language_list)
    local lang_names=`echo ${langs[*]} | sed 's/ /|/g'`
    local total_services=`action_service_show | tail -1 | awk '{print $2}'`

    # 2. get user input and verify them
    while true; do
        read -p "service name? " service

        if $(_check_name "$service"); then
            break;
        fi
    done

    # 3. check whether this is a existing service
    if [ -d "$SKULL_PROJ_ROOT/src/services/$service" ]; then
        echo "Warn: Found the service [$service] has already exist, please" \
            "make sure its a valid service" >&2
        return 1
    fi

    # NOTES: currently, we only support C language
    while true; do
        read -p "which language the service belongs to? ($lang_names) " language

        # verify the language valid or not
        if $(_check_language $langs "$language"); then
            break;
        fi
    done

    while true; do
        read -p "data mode? (exclusive | rw-pr | rw-pw) " data_mode

        if $(__validate_data_mode "$data_mode"); then
            break;
        else
            echo "Fatal: Unknown service data mode: $data_mode, use" \
                "exclusive, rw-pr or rw-pw" >&2
        fi
    done

    # 4. Add basic folder structure if the target module does not exist
    _run_lang_action $language $SKULL_LANG_SERVICE_ADD $service

    # 5. Add module into main config
    $SKULL_ROOT/bin/skull-config-utils.py -m add_service -c $SKULL_CONFIG_FILE \
        -N $service -b true -d $data_mode

    # 6. add common folder
    _run_lang_action $language $SKULL_LANG_COMMON_CREATE

    echo "service [$service] added successfully"
}

function _action_service_config_gen()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    local srv_config=$(_current_service_config $service)
    if [ ! -f $srv_config ]; then
        echo "Error: not found $srv_config" >&2
        exit 1
    fi

    # now, run the config generator
    _run_service_action $SKULL_LANG_GEN_CONFIG $srv_config
}

function _action_service_config_cat()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    local srv_config=$(_current_service_config $service)
    if [ ! -f $srv_config ]; then
        echo "Error: not found $srv_config" >&2
        exit 1
    fi

    cat $srv_config
}

function _action_service_config_edit()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    local srv_config=$(_current_service_config $service)
    if [ ! -f $srv_config ]; then
        echo "Error: not found $srv_config" >&2
        exit 1
    fi

    # TODO: should load a per-user config to identify which editor will be used
    # instead of hardcode `vi` in here
    vim $srv_config
}

function _action_service_config_check()
{
    echo "Error: Unimplemented!" >&2
    exit 1
}

function _action_service_api_list()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    local srv_idl_folder=$SKULL_PROJ_ROOT/src/services/$service/idl
    ls $srv_idl_folder | grep ".proto"
}

function _action_service_api_cat()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    if [ $# = 0 ]; then
        echo "Error: require api name" >&2
        exit 1
    fi

    local idl_name=$1
    local srv_idl_folder=$SKULL_PROJ_ROOT/src/services/$service/idl
    local srv_idl=$srv_idl_folder/$idl_name

    if [ ! -f "$srv_idl" ]; then
        exit 0
    fi

    cat $srv_idl
}

function _action_service_api_edit()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    if [ $# = 0 ]; then
        echo "Error: require api name" >&2
        exit 1
    fi

    local idl_name=$1
    local srv_idl_folder=$SKULL_PROJ_ROOT/src/services/$service/idl
    local srv_idl=$srv_idl_folder/$idl_name

    vim $srv_idl
}

function _action_service_api_check()
{
    echo "Error: Unimplemented!" >&2
    exit 1
}

function _action_service_api_gen()
{
    skull_utils_srv_api_gen

    echo "service api generated, run 'skull build' to re-compile the project"
}

function __validate_api_access_mode()
{
    local access_mode=$1
    local valid_access_modes=('read' 'write' 'read-write')

    for mode in ${valid_access_modes[*]}; do
        if [ "$mode" = "$access_mode" ]; then
            return 0
        fi
    done

    return 1
}

function _action_service_api_add()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    local idl_name=""
    while true; do
        read -p "service api name: " idl_name

        if $(_check_name "$idl_name"); then
            break;
        fi
    done

    local access_mode=""
    while true; do
        read -p "service api access_mode: (read|write|read-write) " access_mode

        if $(__validate_api_access_mode "$access_mode"); then
            break;
        else
            echo "Error: Unknown service api access mode: $access_mode, use" \
                "read, write or read-write" >&2
        fi
    done

    local srv_idl_folder=$SKULL_PROJ_ROOT/src/services/$service/idl
    local template=$SKULL_ROOT/share/skull/template.proto
    local tpl_suffix="proto"

    local idl_req_name=${service}-${idl_name}_req
    local idl_resp_name=${service}-${idl_name}_resp
    local srv_idl_req=$srv_idl_folder/$idl_req_name.$tpl_suffix
    local srv_idl_resp=$srv_idl_folder/$idl_resp_name.$tpl_suffix

    # Generate request proto
    sed "s/TPL_PKG_NAME/$service/g; s/TPL_MSG_NAME/${idl_name}_req/g" $template > $srv_idl_req

    # Generate response proto
    sed "s/TPL_PKG_NAME/$service/g; s/TPL_MSG_NAME/${idl_name}_resp/g" $template > $srv_idl_resp

    # Update skull config
    $SKULL_ROOT/bin/skull-config-utils.py -m add_service_api \
        -c $SKULL_CONFIG_FILE \
        -N $service -a $idl_name -d $access_mode

    echo "$idl_req_name added"
    echo "$idl_resp_name added"
    echo "service api $idl_name added successfully"
}
