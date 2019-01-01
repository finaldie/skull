# This is the utility functions for skull common, which can be easiliy to
# generate the common sources for the supported languages
#
# NOTES: This is included by the main script `skull`

function action_common()
{
    # parse the command args
    local args=`getopt -a -o h -l config-gen,metrics-gen,metrics-dump,idl-gen,help \
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
    echo "  skull common --config-gen"
    echo "  skull common -h|--help"
}

function _action_metrics_gen()
{
    local metrics_config=$SKULL_PROJ_ROOT/config/metrics.yaml
    local langs=($(sk_util_get_language_list))

    for language in ${langs[@]}; do
        echo " - Generating $language counters..."
        action_${language}_gen_metrics $metrics_config
    done
}

function _action_metrics_dump()
{
    cat $SKULL_PROJ_ROOT/config/metrics.yaml
}

function _action_idl_gen()
{
    local langs=($(sk_util_get_language_list))

    # Generate proto source files per-language
    # notes: Different language may use different building folder/structure,
    #  so preparing the building environment job handled by language-
    #  level script
    for language in ${langs[@]}; do
        echo " - Generating $language IDLs..."
        action_${language}_gen_idl $SKULL_CONFIG_FILE
    done
}

function _action_config_gen()
{
    # 1. Generate modules config
    local module_list=($(sk_util_module_list))

    for module in ${module_list[@]}; do
        echo " - Generating module [$module] config..."
        sk_util_module_config_gen $module
    done

    # 2. Generate services config
    local service_list=($(sk_util_service_list))

    for service in ${service_list[@]}; do
        echo " - Generating service [$service] config..."
        sk_util_service_config_gen $service
    done
}
