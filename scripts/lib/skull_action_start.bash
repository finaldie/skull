# This is the utility functions for skull action start:
# example: skull start
#
# NOTES: This is included by the main script `skull`

function action_start()
{
    local start_mode=$1
    local skull_conf=$SKULL_PROJ_ROOT/config/skull-config.yaml
    local deploy_dir=

    if [ $start_mode = "dev" ]; then
        deploy_dir=$SKULL_PROJ_ROOT/run
    elif [ $start_mode = "prod" ]; then
        echo "Fatal: Un-implemented mode"
        exit 1
    else
        echo "Fatal: Unknown start_mode $start_mode"
        exit 1
    fi

    # Fork a process to do the deployment, since we will change the working
    # dir, this will be easily to handle the cleanup job
    (
        if [ $start_mode = "dev" ]; then
            action_deploy dev

            cd $deploy_dir
            skull-engine -c $skull_conf
        elif [ $start_mode = "prod" ]; then
            echo "Fatal: Un-implemented mode"
            exit 1
        else
            echo "Fatal: Unknown start_mode $start_mode"
            exit 1
        fi
    )
}
