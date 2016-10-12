# This is the utility functions for skull deploy:
#
# NOTES: This is included by the main script `skull`

function action_deploy()
{
    local deploy_dir=""
    if [ $# = 0 ]; then
        deploy_dir=$SKULL_PROJ_ROOT/run
    else
        deploy_dir=`readlink -f "$1"`
    fi

    cd $SKULL_PROJ_ROOT
    make deploy DEPLOY_DIR_ROOT=$deploy_dir
    echo "Deploy done, use \"skull start\" to launch the skull-engine"
}

function action_deploy_usage()
{
    echo "usage:"
    echo "  skull deploy [absolute-path]"
}

