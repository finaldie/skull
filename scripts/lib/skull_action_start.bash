# This is the utility functions for skull action start:
# example: skull start
#
# NOTES: This is included by the main script `skull`

function action_start()
{
    deploy_dir=$SKULL_PROJ_ROOT/run

    (
        cd $deploy_dir
        ./bin/skull-start.sh $deploy_dir/skull-config.yaml
    )
}

function action_start_usage()
{
    echo "usage:"
    echo "  skull start"
}
