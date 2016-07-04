# This is the utility functions for skull action start:
# example: skull start
#
# NOTES: This is included by the main script `skull`

function action_start()
{
    # 1. Try to get the project root first if already in a project
    get_proj_root

    # 2. Setup running_dir
    local running_dir="$SKULL_PROJ_ROOT/run"
    if [ $# != 0 ]; then
        if [[ ! "$1" =~ ^- ]]; then
            running_dir="$1"
            shift
        fi
    else
        if [ -z "$SKULL_PROJ_ROOT" ]; then
            echo "Error: Not in a skull project, cannot start a skull-engine" >&2
            exit 1
        fi
    fi

    if [ ! -d "$running_dir" ]; then
        echo "Error: target dir does not exist: $running_dir" >&2
        exit 1
    fi

    # 3. Validate the running_dir is a valid skull project
    local skullbin="$running_dir/bin/skull-start.sh"
    if [ ! -f "$skullbin" ]; then
        echo "Error: $skullbin does not exist" >&2
        exit 1
    fi

    local skullconf="$running_dir/skull-config.yaml"
    if [ ! -f "$skullconf" ]; then
        echo "Error: $skullconf does not exist" >&2
        exit 1
    fi

    # 4. Start skull engine
    cd $running_dir
    exec $skullbin -c "$skullconf" $@
}

function action_start_usage()
{
    echo "usage:"
    echo "  skull start [running_dir] [-D]"
    echo "  skull start [running_dir] --memcheck"
    echo "  skull start [running_dir] --gdb"
    echo "  skull start [running_dir] --strace"
    echo "  skull start [running_dir] --massif"
}
