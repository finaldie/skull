# This is a utility functions for skull action create :
# It will create a skull project, example:
# 1. skull create test_proj
# 2. skull create ./
#
# NOTES: This is included by the main script `skull`

function _skull_create()
{
    local workspace=$1
    # build the basic workspace folder structure
    mkdir -p $workspace
    mkdir -p $workspace/.skull
    mkdir -p $workspace/src/modules
    mkdir -p $workspace/src/common
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
    cp -R $SKULL_ROOT/etc/skull/* $workspace/config
}

function action_create()
{
    if [ $# = 0 ]; then
        echo "Error: missing project name" >&2
        print_usage >&2
        exit 1
    fi

    local workspace=$1
    echo "create skull workspace..."
    if [ -d $workspace ] && [ -d $workspace/.skull ]; then
        echo "Notice: The workspace [$workspace] is a skull project, " \
             "give up to build it"
        exit 1
    fi

    _skull_create $workspace
    echo "create skull workspace done"
}
