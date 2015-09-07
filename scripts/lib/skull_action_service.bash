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
        -l add,show,help,conf-gen,conf-cat,conf-edit,conf-check,idl-list,idl-add,idl-cat,idl-edit,idl-check,idl-gen \
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
            -s|--show)
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
            --idl-list)
                shift
                _action_service_idl_list
                exit 0
                ;;
            --idl-add)
                shift 2
                _action_service_idl_add $@
                exit 0
                ;;
            --idl-cat)
                shift 2
                _action_service_idl_cat $@
                exit 0
                ;;
            --idl-edit)
                shift 2
                _action_service_idl_edit $@
                exit 0
                ;;
            --idl-check)
                shift 2
                _action_service_idl_check
                exit 0
                ;;
            --idl-gen)
                shift 2
                _action_service_idl_gen
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
    echo "  skull service -s|--show"
    echo "  skull service -h|--help"
    echo "  skull service --conf-gen"
    echo "  skull service --conf-cat"
    echo "  skull service --conf-edit"
    echo "  skull service --conf-check"

    echo "  skull service --idl-list"
    echo "  skull service --idl-add"
    echo "  skull service --idl-cat"
    echo "  skull service --idl-edit"
    echo "  skull service --idl-gen"
    echo "  skull service --idl-check"
}

function action_service_show()
{
    $SKULL_ROOT/bin/skull-workflow.py -m show_service -c $SKULL_CONFIG_FILE
}

function _action_service_add()
{
    # 1. input the module name
    local service=""
    local language=""
    local langs=$(_get_language_list)
    local lang_names=`echo ${langs[*]} | sed 's/ /|/g'`
    local total_services=`action_service_show | tail -1 | awk '{print $2}'`

    # 2. get user input and verify them
    while true; do
        read -p "service name? " service

        if $(_check_name $service); then
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
        read -p "which language the service belongs to?($lang_names) " language

        # verify the language valid or not
        if $(_check_language $langs $language); then
            break;
        fi
    done

    # 4. Add basic folder structure if the target module does not exist
    _run_lang_action $language $SKULL_LANG_SERVICE_ADD $service

    # 5. Add module into main config
    $SKULL_ROOT/bin/skull-workflow.py -m add_service -c $SKULL_CONFIG_FILE \
        -N $service -b true

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

function _action_service_idl_list()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    local srv_idl_folder=$SKULL_PROJ_ROOT/src/services/$service/idl
    ls $srv_idl_folder | grep ".proto"
}

function _action_service_idl_cat()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    if [ $# = 0 ]; then
        echo "Error: require idl name" >&2
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

function _action_service_idl_edit()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    if [ $# = 0 ]; then
        echo "Error: require idl name" >&2
        exit 1
    fi

    local idl_name=$1
    local srv_idl_folder=$SKULL_PROJ_ROOT/src/services/$service/idl
    local srv_idl=$srv_idl_folder/$idl_name

    vim $srv_idl
}

function _action_service_idl_check()
{
    echo "Error: Unimplemented!" >&2
    exit 1
}

function _action_service_idl_gen()
{
    skull_utils_srv_api_gen
}

function _action_service_idl_add()
{
    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service" >&2
        exit 1
    fi

    if [ $# = 0 ]; then
        echo "Error: require idl name" >&2
        exit 1
    fi

    local idl_name=$1
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

    echo "$idl_req_name added"
    echo "$idl_resp_name added"
}
