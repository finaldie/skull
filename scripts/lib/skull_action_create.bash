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
    mkdir -p $workspace/.skull/makefiles
    mkdir -p $workspace/idls/workflow
    mkdir -p $workspace/idls/service
    mkdir -p $workspace/src/modules
    mkdir -p $workspace/src/services
    mkdir -p $workspace/src/common
    mkdir -p $workspace/scripts
    mkdir -p $workspace/tests
    mkdir -p $workspace/config
    mkdir -p $workspace/bin

    # copy templates to the target workspace
    cp $SKULL_ROOT/share/skull/Makefile.tpl $workspace/Makefile
    cp $SKULL_ROOT/share/skull/ChangeLog.md.tpl $workspace/ChangeLog.md
    cp $SKULL_ROOT/share/skull/README.md.tpl $workspace/README.md
    cp -r $SKULL_ROOT/share/skull/bin/* $workspace/bin
    cp $SKULL_ROOT/share/skull/gitignore $workspace/.gitignore
    cp $SKULL_ROOT/share/skull/ycm_extra_conf.py $workspace/.ycm_extra_conf.py

    # copy all the configurations except ChangeLog.md
    local copy_list=`find $SKULL_ROOT/etc/skull/* -name "*" | grep -v "ChangeLog.md"`
    cp -r $copy_list $workspace/config
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

function action_create_usage()
{
    echo "usage:"
    echo "  skull create project"
    echo "  skull create ./"
}
