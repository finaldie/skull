# This is the utility functions for skull deploy:
#
# NOTES: This is included by the main script `skull`

function action_deploy()
{
    # parse the command args
    local args=`getopt -a -o ph -l prod,help \
        -n "skull_action_deploy.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_deploy_usage
        exit 1
    fi

    eval set -- "$args"

    # If there is only 1 arg, means there is no user input arg,
    # so  by default, we deploy it on the $SKULL_PROJ_ROOT/run
    if [ $# = 1 ]; then
        _action_deploy_dev
        echo "Deploy done, use \"skull start\" to launch the skull-engine"
        exit 0
    fi

    # parse and run the command
    while true; do
        case "$1" in
            -p|--prod)
                shift
                _action_deploy_prod
                exit 0
                ;;
            -h|--help)
                shift
                action_deploy_usage >&2
                exit 0
                ;;
            --)
                shift; break
                exit 0
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_deploy_usage >&2
                exit 1
                ;;
        esac
    done

}

function action_deploy_usage()
{
    echo "usage:"
    echo "  skull deploy"
}

function _action_deploy_dev()
{
    # Fork a process to do the deployment, since we will change the working
    # dir, this will be easily to handle the cleanup job
    (
        cd $SKULL_PROJ_ROOT
        local deploy_dir=$SKULL_PROJ_ROOT/run
        make deploy DEPLOY_DIR_ROOT=$deploy_dir
    )
}

function _action_deploy_prod()
{
    echo "Fatal: Un-implemented mode" >&2
    exit 1
}
