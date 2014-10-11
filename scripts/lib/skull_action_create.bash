# This is a utility functions for skull action create :
# It will create a skull project, example:
# 1. skull create test_proj
# 2. skull create ./
#
# NOTES: This is included by the main script `skull`

function skull_create()
{
    local workspace=$1
    # build the basic workspace folder structure
    mkdir -p $workspace
    mkdir -p $workspace/.skull
    mkdir -p $workspace/components/modules
    mkdir -p $workspace/components/common
    mkdir -p $workspace/scripts
    mkdir -p $workspace/tests
    mkdir -p $workspace/config
    mkdir -p $workspace/bin

    # copy templates to the target workspace
    cp $SKULL_ROOT/share/skull/Makefile.tpl $workspace/Makefile
    cp $SKULL_ROOT/share/skull/ChangeLog.md.tpl $workspace/ChangeLog.md
    cp $SKULL_ROOT/share/skull/README.md.tpl $workspace/README.md
    cp -R $SKULL_ROOT/share/skull/bin/* $workspace/bin
    cp -R $SKULL_ROOT/share/skull/scripts/* $workspace/scripts
    cp $SKULL_ROOT/etc/skull/skull-config.yaml $workspace/config/skull-config.yaml
}

function action_create()
{
    if [ $# = 0 ]; then
        echo "Error: missing project name"
        print_usage
        exit 1
    fi

    local workspace=$1
    echo "create skull workspace..."
    if [ -d $workspace ] && [ -d $workspace/.skull ]; then
        echo "Notice: The workspace [$workspace] is a skull project, " \
             "give up to build it"
        exit 1
    fi

    skull_create $workspace
    echo "create skull workspace done"
}
