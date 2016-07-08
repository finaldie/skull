# This is the utility functions for skull common, which can be easiliy to
# generate the common sources for the supported languages
#
# NOTES: This is included by the main script `skull`

function action_common()
{
    # parse the command args
    local args=`getopt -a -o h -l config-gen,metrics-gen,metrics-dump,idl-gen,srv-idl-gen,help \
        -n "skull_action_common.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_common_usage >&2
        exit 1
    fi

    eval set -- "$args"

    while true; do
        case "$1" in
            --metrics-gen)
                shift
                _action_metrics_gen
                ;;
            --metrics-dump)
                shift
                _action_metrics_dump
                ;;
            --idl-gen)
                shift
                _action_idl_gen
                ;;
            --srv-idl-gen)
                shift
                _action_srv_idl_gen
                ;;
            --config-gen)
                shift
                _action_config_gen
                ;;
            -h|--help)
                shift
                action_common_usage >&2
                ;;
            --)
                shift; break
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_common_usage >&2
                ;;
        esac
    done
}

function action_common_usage()
{
    echo "usage: "
    echo "  skull common --metrics-gen"
    echo "  skull common --metrics-dump"
    echo "  skull common --idl-gen"
    echo "  skull common --srv-idl-gen"
    echo "  skull common -h|--help"
}

function _action_metrics_gen()
{
    local metrics_config=$SKULL_PROJ_ROOT/config/metrics.yaml
    local langs=($(_get_language_list))

    for language in ${langs[@]}; do
        if [ -d "$SKULL_PROJ_ROOT/src/common/$language" ]; then
            echo " - generating $language metrcis..."
            action_${language}_gen_metrics $metrics_config
        fi
    done
}

function _action_metrics_dump()
{
    cat $SKULL_PROJ_ROOT/config/metrics.yaml
}

function _action_idl_gen()
{
    local langs=($(_get_language_list))

    for language in ${langs[@]}; do
        action_${language}_gen_idl $SKULL_CONFIG_FILE
    done
}

function _action_srv_idl_gen()
{
    skull_utils_srv_api_gen
}

function _action_config_gen()
{
    # 1. Generate modules config
    for module in $(_module_list); do
        echo " - Generating module [$module] config..."
        utils_module_config_gen $module
    done

    # 2. Generate services config
    for service in $(utils_service_list); do
        echo " - Generating service [$service] config..."
        utils_service_config_gen $service
    done
}
