# This is the utility functions for skull action start:
# example: skull start
#
# NOTES: This is included by the main script `skull`

function action_start()
{
    local deploy_dir=""
    if [ $# = 0 ]; then
        deploy_dir=$SKULL_PROJ_ROOT/run
    else
        deploy_dir="$1"
    fi

    if [ ! -d "$deploy_dir" ]; then
        echo "Error: target dir do not exist: $deploy_dir" >&2
        exit 1
    fi

    cd $SKULL_PROJ_ROOT
    exec ./bin/skull-start.sh -c $deploy_dir/skull-config.yaml $@
}

function action_start_usage()
{
    echo "usage:"
    echo "  skull start"
    echo "  skull start --memcheck"
    echo "  skull start --gdb"
    echo "  skull start --strace"
    echo "  skull start --massif"
}
