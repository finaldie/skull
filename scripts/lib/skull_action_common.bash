# This is the utility functions for skull common, which can be easiliy to
# generate the common sources for the supported languages
#
# NOTES: This is included by the main script `skull`

function action_common()
{
    # parse the command args
    local args=`getopt -a -o h -l metrics-gen,metrics-dump,idl-gen,srv-idl-gen,help \
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
    local langs=$(_get_language_list)

    for language in $langs; do
        action_${language}_gen_metrics $metrics_config
    done
}

function _action_metrics_dump()
{
    cat $SKULL_PROJ_ROOT/config/metrics.yaml
}

function _action_idl_gen()
{
    local skull_config=$SKULL_PROJ_ROOT/config/skull-config.yaml
    local langs=$(_get_language_list)

    for language in $langs; do
        action_${language}_gen_idl $skull_config
    done
}

function _action_srv_idl_gen()
{
    skull_utils_srv_api_gen
}
