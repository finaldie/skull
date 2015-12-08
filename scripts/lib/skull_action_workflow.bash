# This is a utility functions for skull action workflow :
# Add a workflows into a skull project, example:
#  skull workflow --add
#
# NOTES: This is included by the main script `skull`

function action_workflow()
{
    # parse the command args
    local args=`getopt -a -o alh -l add,list,help \
        -n "skull_action_workflow.bash" -- "$@"`
    if [ $? != 0 ]; then
        echo "Error: Invalid parameters" >&2
        action_workflow_usage
        exit 1
    fi

    eval set -- "$args"

    while true; do
        case "$1" in
            -a|--add)
                shift
                _action_workflow_add
                exit 0
                ;;
            -l|--list)
                shift
                action_workflow_show
                exit 0
                ;;
            -h|--help)
                shift
                action_workflow_usage >&2
                exit 0
                ;;
            --)
                shift; break
                exit 0
                ;;
            *)
                echo "Error: Invalid parameters $1" >&2
                shift
                action_workflow_usage >&2
                exit 1
                ;;
        esac
    done
}

function action_workflow_usage()
{
    echo "usage: "
    echo "  skull workflow -a|--add"
    echo "  skull workflow -l|--list"
    echo "  skull workflow -h|--help"
}

function _action_workflow_add()
{
    # prepare add workflow
    local skull_conf=$SKULL_CONFIG_FILE
    local concurrent=1
    local idl=""
    local port=1234

    local yn_concurrent=true
    local yn_port=true

    # set the concurrent
    read -p "Whether allow concurrent? (y/n) " yn_concurrent
    if [ ! "$yn_concurrent" = "y" ]; then
        concurrent=0
    fi

    # set idl
    while true; do
        read -p "input the idl name: " idl

        if [ -z "$idl" ]; then
            echo "Error: please input a non-empty idl name" >&2
        else
            break
        fi
    done

    # set the port
    read -p "Need listen on a port? (y/n) " yn_port

    if [ "$yn_port" = "y" ]; then
        while true; do
            read -p "Input the port you want (1025-65535): " port

            if ! $(_is_number $port); then
                echo "Error: please input a digital for the port" >&2
            else
                break
            fi
        done
    fi

    # add workflow into skull-config.yaml
    $SKULL_ROOT/bin/skull-config-utils.py -m add_workflow \
        -c $skull_conf -C $concurrent -i $idl -p $port

    # generate the workflow txn idl file if it's not exist
    local idl_path=$SKULL_WORKFLOW_IDL_FOLDER
    if [ ! -f $SKULL_WORKFLOW_IDL_FOLDER/${idl}.proto ]; then
        $SKULL_ROOT/bin/skull-config-utils.py -m generate_workflow_idl \
            -c $skull_conf -n $idl -p $idl_path
    fi

    echo "workflow added successfully"
    echo "note: run 'skull module --add' to create a new module for it"
}

function action_workflow_show()
{
    local skull_conf=$SKULL_PROJ_ROOT/config/skull-config.yaml

    $SKULL_ROOT/bin/skull-config-utils.py -m show_workflow -c $skull_conf
}
