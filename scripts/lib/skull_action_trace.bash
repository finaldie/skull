# This is the utility functions for skull action trace:
# example: skull trace -p 12345
#
# NOTES: This is included by the main script `skull`

function action_trace()
{
    # parse the command args
    local args=`getopt -a -o p:n:fdh -l help \
        -n "skull_action_trace.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_trace_usage
        exit 1
    fi

    eval set -- "$args"

    local port=-1
    local count=10 # Default last 10 lines
    local follow=false
    local debug=false

    while true; do
        case "$1" in
            -p)
                shift
                port=$1
                shift
                ;;
            -n)
                shift
                count=$1
                shift
                ;;
            -f)
                shift
                follow=true
                ;;
            -d)
                shift
                debug=true
                ;;
            -h|--help)
                shift
                action_trace_usage
                exit 0
                ;;
            --)
                shift;
                break;
                exit 0
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_trace_usage >&2
                exit 1
                ;;
        esac
    done

    do_trace $port $count $follow $debug
}

function do_trace() {
    local port=$1
    local count=$2
    local follow=$3
    local debug=$4

    if ! $(sk_util_is_number $port); then
        echo "Error: Invalid port $port" >&2
        exit 1
    fi

    if ! $(sk_util_is_number $count); then
        echo "Error: Invalid count $count" >&2
        exit 1
    fi

    # 1. Get skull info via port
    echo "Connecting to skull-engine :$port ..."
    local infos=`echo "info" | nc 0 7759 -w 1`
    if [ $? -ne 0 ] || [ -z "$infos" ]; then
        echo " - Error in getting infos from port $port, check whether skull-engine is running" >&2
        exit 1
    fi

    # 2. Get pid from info output
    local pid=$(_get_value_from_info "$infos" "pid")
    echo " - Got pid: $pid"

    # 3. Get std_forward
    local std_forward=$(_get_value_from_info "$infos" "log_std_forward")
    echo " - Got std_forward: $std_forward"

    # 4. Get diag file
    local diag="/dev/null"
    if [ $std_forward -eq 1 ]; then
        diag=$(_get_value_from_info "$infos" "stdout")
    else
        diag=$(_get_value_from_info "$infos" "diag_file")
    fi
    echo " - Got diag_file: $diag"

    # 5. Get skull-engine
    local skull_bin=$(_get_value_from_info "$infos" "binary")
    echo " - Got skull-engine: $skull_bin"

    if $follow; then
        tail -F $diag | grep "[MEM_TRACE]" | skull-addr-remap.py -e $skull_bin -p $pid
    else
        tail -n $count $diag | grep "[MEM_TRACE]" | skull-addr-remap.py -e $skull_bin -p $pid
    fi
}

function _get_value_from_info() {
    local infos="$1"
    local key="$2"

    local value=`echo "$infos" | grep "$key" | awk '{print $3}'`
    if [ $? -ne 0 ]; then
        echo "Error in getting $key, check whether skull-engine is running" >&2
        exit 1
    fi
    echo "$value"
}

function action_trace_usage() {
    echo "usage:"
    echo "  skull trace -p port [-d] [-n count] [-f]"
}
