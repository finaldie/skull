# This is the utility functions for skull build, which can be easiliy to build
# the entire project without must move to the top folder, meanwhile you can
# build the project any where when you in a skull project
#
# NOTES: This is included by the main script `skull`

function action_config()
{
    # parse the command args
    local args=`getopt -a \
        -o eh \
        -l edit,help \
        -n "skull_action_module.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_config_usage >&2
        exit 1
    fi

    eval set -- "$args"

    while true; do
        case "$1" in
            -e|--edit)
                shift
                _action_config_edit
                exit 0
                ;;
            -h|--help)
                shift
                action_config_usage >&2
                ;;
            --)
                shift;
                # if there is no parameter left, show the modules directly
                if [ $# = 0 ]; then
                    _action_config_dump
                fi
                break;
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_config_usage >&2
                exit 1
                ;;
        esac
    done
}

function action_config_usage()
{
    echo "usage: "
    echo "  skull config"
    echo "  skull config -e|--edit"
}

function _action_config_dump()
{
    cat $SKULL_CONFIG_FILE
}

function _action_config_edit()
{
    vi $SKULL_CONFIG_FILE
}
