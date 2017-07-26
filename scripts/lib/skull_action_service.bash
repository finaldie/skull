# This is the utility functions for skull action module:
# example: skull service --add
#
# NOTES: This is included by the main script `skull`

# Add a service need 2 steps:
# 1. Add the service folder structure into project according to its language
# 2. Change the main config

#set -x

function action_service()
{
    # parse the command args
    local args=`getopt -a \
        -o lh \
        -l add,list,help,conf-gen,conf-cat,conf-edit,conf-check,api-list,api-add,api-cat:,api-edit:,api-check,api-gen,import: \
        -n "skull_action_service.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_service_usage >&2
        exit 1
    fi

    eval set -- "$args"

    while true; do
        case "$1" in
            --add)
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
                shift
                _action_service_api_add
                exit 0
                ;;
            --api-cat)
                _action_service_api_cat "$2"
                shift 2
                exit 0
                ;;
            --api-edit)
                _action_service_api_edit "$2"
                shift 2
                exit 0
                ;;
            --api-check)
                shift
                _action_service_api_check
                exit 0
                ;;
            --api-gen)
                shift
                _action_service_api_gen
                exit 0
                ;;
            --import)
                _action_service_import "$2"
                shift 2
                exit 0
                ;;
            --)
                shift;
                if [ $# = 0 ]; then
                    action_service_show
                fi
                break;
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

    if [ $# = 0 ]; then
        exit 0;
    fi

    local service_name="$1"
    _action_service_path "$service_name"
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

    echo "  skull service --import"
}

function action_service_show()
{
    $SKULL_ROOT/bin/skull-config-utils.py -m service -c $SKULL_CONFIG_FILE -a show
}

function __validate_data_mode()
{
    local data_mode=$1
    local valid_data_modes=('rw-pr' 'rw-pw')

    for mode in ${valid_data_modes[*]}; do
        if [ "$mode" = "$data_mode" ]; then
            return 0
        fi
    done

    return 1
}

function _action_service_path()
{
    if [ $# = 0 ]; then
        echo "Error: empty service name" >&2
        exit 1
    fi

    local service_name="$1"
    local service_path="$SKULL_PROJ_ROOT/src/services/$service_name"
    echo "service path: $service_path"
}

function _action_service_add()
{
    # 1. input the module name
    local service=""
    local language=""
    local data_mode=""
    local langs=($(sk_util_get_language_list))
    local lang_names=`echo ${langs[*]} | sed 's/ /|/g'`
    local total_services=`action_service_show | tail -1 | awk '{print $2}'`

    # 2. get user input and verify them
    while true; do
        read -p "Service Name? " service

        if $(sk_util_check_name "$service"); then
            break;
        fi
    done

    # 3. check whether this is a existing service
    local service_path="$SKULL_PROJ_ROOT/src/services/$service"
    if [ -d "$service_path" ]; then
        echo "Warn: Found the service [$service] has already exist, please" \
            "make sure its a valid service" >&2
        return 1
    fi

    # NOTES: currently, we only support Cpp language
    while true; do
        read -p "Service Language? ($lang_names) " language

        # verify the language valid or not
        if $(sk_util_check_language "$language"); then
            break;
        fi
    done

    while true; do
        read -p "Data Mode? (rw-pr | rw-pw) " data_mode

        if $(__validate_data_mode "$data_mode"); then
            break;
        else
            echo "Fatal: Unknown service data mode: $data_mode, use" \
                "rw-pr or rw-pw" >&2
        fi
    done

    # 4. Add basic folder structure if the target module does not exist
    sk_util_run_lang_action $language $SKULL_LANG_SERVICE_ADD $service

    if [ $? != 0 ]; then
        echo "Error: Service creation failed" >&2
        exit 1
    fi

    # 5. Add service into main config
    $SKULL_ROOT/bin/skull-config-utils.py -m service -c $SKULL_CONFIG_FILE \
        -a add -s $service -b true -d $data_mode -l $language

    # 6. add common folder
    sk_util_run_lang_action $language $SKULL_LANG_COMMON_CREATE

    echo "Service [$service] added successfully"
}

function _action_service_config_gen()
{
    local service=$(sk_util_current_service)
    if [ -z "$service" ]; then
        echo "Error: Not in a service, pwd: `pwd`" >&2
        exit 1
    fi

    sk_util_service_config_gen $service
}

function _action_service_config_cat()
{
    local service=$(sk_util_current_service)
    if [ -z "$service" ]; then
        echo "Error: Not in a service, pwd: `pwd`" >&2
        exit 1
    fi

    local srv_config=$(sk_util_current_service_config $service)
    if [ ! -f $srv_config ]; then
        echo "Error: not found $srv_config" >&2
        exit 1
    fi

    cat $srv_config
}

function _action_service_config_edit()
{
    local service=$(sk_util_current_service)
    if [ -z "$service" ]; then
        echo "Error: Not in a service, pwd: `pwd`" >&2
        exit 1
    fi

    local srv_config=$(sk_util_current_service_config $service)
    if [ ! -f $srv_config ]; then
        echo "Error: Not found $srv_config" >&2
        exit 1
    fi

    # TODO: should load a per-user config to identify which editor will be used
    # instead of hardcode `vi` in here
    $SKULL_DEFAULT_EDITOR $srv_config
}

function _action_service_config_check()
{
    echo "Error: Unimplemented!" >&2
    exit 1
}

function _action_service_api_list()
{
    local service=$(sk_util_current_service)
    if [ -z "$service" ]; then
        echo "Error: Not in a service, pwd: `pwd`" >&2
        exit 1
    fi

    local srv_idl_folder=$SKULL_PROJ_ROOT/src/services/$service/idl
    ls $srv_idl_folder | grep ".proto"
}

function _action_service_api_cat()
{
    local service=$(sk_util_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service, pwd: `pwd`" >&2
        exit 1
    fi

    if [ $# = 0 ]; then
        echo "Error: Requires an API name" >&2
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
    local service=$(sk_util_current_service)
    if [ -z "$service" ]; then
        echo "Error: Not in a service, pwd: `pwd`" >&2
        exit 1
    fi

    if [ $# = 0 ]; then
        echo "Error: Requires an API name" >&2
        exit 1
    fi

    local idl_name=$1
    local srv_idl_folder=$SKULL_PROJ_ROOT/src/services/$service/idl
    local srv_idl=$srv_idl_folder/$idl_name

    $SKULL_DEFAULT_EDITOR $srv_idl
}

function _action_service_api_check()
{
    echo "Error: Unimplemented!" >&2
    exit 1
}

function _action_service_api_gen()
{
    action_common --idl-gen || exit 1

    echo "Service API generation complete. Run 'skull build' to re-compile the project"
}

function _action_service_api_add()
{
    local service=$(sk_util_current_service)
    if [ -z "$service" ]; then
        echo "Error: Not in a service, pwd: `pwd`" >&2
        exit 1
    fi

    local idl_name=""
    while true; do
        read -p "Service API Name: " idl_name

        if $(sk_util_check_name "$idl_name"); then
            break;
        fi
    done

    local service_path="$SKULL_PROJ_ROOT/src/services/$service"
    local srv_idl_folder=$service_path/idl
    local template_req=$SKULL_ROOT/share/skull/service_req.proto
    local template_resp=$SKULL_ROOT/share/skull/service_resp.proto
    local tpl_suffix="proto"

    local idl_req_name=${service}-${idl_name}_req
    local idl_resp_name=${service}-${idl_name}_resp
    local srv_idl_req=$srv_idl_folder/$idl_req_name.$tpl_suffix
    local srv_idl_resp=$srv_idl_folder/$idl_resp_name.$tpl_suffix

    # Generate request proto
    sed "s/TPL_PKG_NAME/$service/g; s/TPL_MSG_NAME/${idl_name}_req/g" $template_req > $srv_idl_req

    # Generate response proto
    sed "s/TPL_PKG_NAME/$service/g; s/TPL_MSG_NAME/${idl_name}_resp/g" $template_resp > $srv_idl_resp

    echo "$idl_req_name added"
    echo "$idl_resp_name added"
    echo "Service API $idl_name added successfully"
}

function _action_service_import()
{
    if [ $# = 0 ]; then
        echo "Error: Please input a service name" >&2
        exit 1
    fi

    local service="$1"
    local service_path="$SKULL_PROJ_ROOT/src/services/$service"
    local data_mode="rw-pr"
    local language=""

    # 1. Whether the service folder exist
    if [ ! -d "$service_path" ]; then
        echo "Error: Service '$service' doesn't exist" >&2
        exit 1
    fi

    # 2. Check whehter it's in skull.config
    $SKULL_ROOT/bin/skull-config-utils.py -m service \
        -c $SKULL_CONFIG_FILE -a exist -s $service

    if [ $? -eq 0 ]; then
        echo "Info: Service '$service' has already imported"
        exit 0
    fi

    # 3. Get service data mode
    while true; do
        read -p "Data Mode? (rw-pr | rw-pw) " data_mode

        if $(__validate_data_mode "$data_mode"); then
            break;
        else
            echo "Fatal: Unknown service data mode: $data_mode, use" \
                "rw-pr or rw-pw" >&2
        fi
    done

    # 4. Get service language
    language=$(sk_util_service_language $service)
    if [ -z "$language" ]; then
        echo "Error: Cannot detect service language, import service failed" >&2
        exit 1
    fi

    # 5. Add common folder if possible
    sk_util_run_lang_action $language $SKULL_LANG_COMMON_CREATE

    # 6. Generate service config related code
    sk_util_service_config_gen $service
    if [ $? != 0 ]; then
        echo "Error: Import service failed, cannot generate config" >&2
        exit 1
    fi

    # 7. Add service into skull-config
    $SKULL_ROOT/bin/skull-config-utils.py -m service -c $SKULL_CONFIG_FILE \
        -a add -s $service -b true -d $data_mode -l $language

    if [ $? = 0 ]; then
        echo "Import service successfully"
        exit 0
    else
        echo "Error: Import service failed" >&2
        exit 1
    fi
}

