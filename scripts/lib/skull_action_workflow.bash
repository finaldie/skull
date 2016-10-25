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
                action_workflow_usage
                exit 0
                ;;
            --)
                shift;
                action_workflow_show
                break;
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
    echo "Usage: "
    echo "  skull workflow -a|--add"
    echo "  skull workflow -l|--list"
    echo "  skull workflow -h|--help"
}

function _action_workflow_add()
{
    # 1. prepare add workflow
    local skull_conf=$SKULL_CONFIG_FILE
    local concurrent=1
    local enable_stdin=0
    local idl=""
    local port=-1

    local yn_concurrent=true
    local yn_stdin=true
    local yn_port=true

    # 2. set the concurrent
    read -p "Allow concurrent? (y/n) " yn_concurrent
    if ! $(sk_util_yn_yes "$yn_concurrent"); then
        concurrent=0
    fi

    # 3. set idl
    while true; do
        read -p "Input the IDL name: " idl

        if [ -z "$idl" ]; then
            echo "Error: please input a non-empty idl name" >&2
        else
            break
        fi
    done

    # 4. set trigger
    ## 4.1 set the stdin
    read -p "Input Data Source: stdin? (y/n) " yn_stdin
    if ! $(sk_util_yn_valid "$yn_stdin"); then
        echo "Error: Type 'y' or 'n'" >&2
        exit 1
    fi

    if $(sk_util_yn_yes "$yn_stdin"); then
        enable_stdin=1
    else
        ## 4.2 set the port
        read -p "Input Data Source: Network? (y/n) " yn_port
        if ! $(sk_util_yn_valid "$yn_port"); then
            echo "Error: Type 'y' or 'n'" >&2
            exit 1
        fi

        if $(sk_util_yn_yes "$yn_port"); then
            while true; do
                read -p "Input the port you want (1025-65535): " port

                if ! $(sk_util_is_number $port); then
                    echo "Error: please input a digital for the port" >&2
                else
                    break
                fi
            done
        fi
    fi

    # 5. add workflow into skull-config.yaml
    $SKULL_ROOT/bin/skull-config-utils.py -m workflow \
        -c $skull_conf -a add -C $concurrent -i $idl -p $port -I $enable_stdin

    # 6. generate the workflow txn idl file if it's not exist
    local idl_path=$SKULL_WORKFLOW_IDL_FOLDER
    if [ ! -f $SKULL_WORKFLOW_IDL_FOLDER/${idl}.proto ]; then
        $SKULL_ROOT/bin/skull-config-utils.py -m workflow \
            -c $skull_conf -a gen_idl -n $idl -p $idl_path
    fi

    echo "Workflow added successfully"
    echo ""
    echo "Note: All the workflow idls are in 'idls' folder"
    echo "Note: Run 'skull module --add' to create a new module for it"
}

function action_workflow_show()
{
    $SKULL_ROOT/bin/skull-config-utils.py -m workflow \
        -c $SKULL_CONFIG_FILE -a show
}
