# This is the utility functions for skull metrics, which can be easiliy to
# generate the metrics for the supported languages
#
# NOTES: This is included by the main script `skull`

function action_metrics()
{
    # parse the command args
    local args=`getopt -a -o gdh -l gen,dump,help \
        -n "skull_action_metrics.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_module_usage >&2
        exit 1
    fi

    eval set -- "$args"

    while true; do
        case "$1" in
            -g|--gen)
                shift
                _action_metrics_gen
                ;;
            -d|--dump)
                shift
                _action_metrics_dump
                ;;
            -h|--help)
                shift
                action_metrics_usage >&2
                ;;
            --)
                shift; break
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_metrics_usage >&2
                ;;
        esac
    done
}

function action_metrics_usage()
{
    echo "usage: "
    echo "  skull metrics -g|--gen"
    echo "  skull metrics -d|--dump"
    echo "  skull metrics -h|--help"
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
