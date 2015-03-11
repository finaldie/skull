# This is the bash script for C language
# NOTES: This script must implement the following functions:
#  - `action_$lang_module_valid`
#  - `action_$lang_module_add`
#  - `action_$lang_common_create`
#  - `action_$lang_gen_metrics`
#  - `action_$lang_gen_config`
#  - `action_$lang_gen_idl`
#
# NOTES2: before running any methods in file, skull will change the current
# folder to the top of the project `SKULL_PROJECT_ROOT`, so it's no need to
# change it again manually

# static env var
LANGUAGE_PATH=$SKULL_ROOT/share/skull/lang/c
COMMON_FILE_LOCATION=$SKULL_PROJ_ROOT/src/common/c

# return:
#  - 0 if this is a valid C module
#  - 1 if this is not a valid C module
function action_c_module_valid()
{
    # for now, just check whether there is mod.c exist
    local module=$1
    if [ -f $SKULL_PROJ_ROOT/src/modules/$module/src/mod.c ]; then
        return 0
    else
        return 1
    fi
}

function action_c_module_add()
{
    local module=$1
    local module_path=$SKULL_PROJ_ROOT/src/modules/$module

    if [ -d "$module_path" ]; then
        echo "Notice: the module [$module] has already exist"
        return 1
    fi

    # add module folder and files
    mkdir -p $module_path/src
    mkdir -p $module_path/tests
    mkdir -p $module_path/config
    mkdir -p $module_path/lib

    cp $LANGUAGE_PATH/share/mod.c.tpl        $module_path/src/mod.c
    cp $LANGUAGE_PATH/etc/config.yaml        $module_path/config/config.yaml
    cp $LANGUAGE_PATH/share/test_mod.c.tpl   $module_path/tests/test_mod.c
    cp $LANGUAGE_PATH/etc/test_config.yaml   $module_path/tests/test_config.yaml
    cp $LANGUAGE_PATH/share/gitignore-module $module_path/.gitignore

    # copy makefile templates
    cp $LANGUAGE_PATH/share/Makefile.tpl $module_path/Makefile
    cp $LANGUAGE_PATH/share/Makefile.inc.tpl $SKULL_MAKEFILE_FOLDER/Makefile.c.inc
    cp $LANGUAGE_PATH/share/Makefile.targets.tpl $SKULL_MAKEFILE_FOLDER/Makefile.c.targets

    # generate a static config code for user
    local module_config=$module_path/config/config.yaml
    action_c_gen_config $module_config

    return 0
}

function action_c_common_create()
{
    if [ -d "$COMMON_FILE_LOCATION" ]; then
        echo "notice: the common/c folder has already exist, ignore it"
        return 0
    fi

    # create common folders
    mkdir -p $COMMON_FILE_LOCATION/src
    mkdir -p $COMMON_FILE_LOCATION/tests
    mkdir -p $COMMON_FILE_LOCATION/lib

    # move the Makefile to common/c only when there is no Makefile in common/c
    if [ ! -f $COMMON_FILE_LOCATION/Makefile ]; then
        cp $LANGUAGE_PATH/share/Makefile.common.tpl \
            $COMMON_FILE_LOCATION/Makefile
    fi

    # copy gitignore
    if [ ! -f $COMMON_FILE_LOCATION/.gitignore ]; then
        cp $LANGUAGE_PATH/share/gitignore-common \
            $COMMON_FILE_LOCATION/.gitignore
    fi

    # copy common makefile targets
    cp $LANGUAGE_PATH/share/Makefile.common.targets.tpl \
        $SKULL_MAKEFILE_FOLDER/Makefile.common.c.targets

    # generate the metrics
    action_c_gen_metrics $SKULL_METRICS_FILE

    # generate idl source code according to the idls
    local config=$SKULL_PROJ_ROOT/config/skull-config.yaml
    action_c_gen_idl $config

    # copy the unit test file
    cp $LANGUAGE_PATH/share/test_common.c.tpl \
        $COMMON_FILE_LOCATION/tests/test_common.c
}

function action_c_gen_metrics()
{
    local config=$1

    # TODO: compare the md5 of the new metrics and old metrics' files, do not to
    # replace them if they are same, it will reduce the compiling time
    $LANGUAGE_PATH/bin/skull-metrics-gen.py -c $config \
        -h $COMMON_FILE_LOCATION/src/skull_metrics.h \
        -s $COMMON_FILE_LOCATION/src/skull_metrics.c
}

function action_c_gen_config()
{
    local config=$1
    local confdir=`dirname $config`
    local targetdir=$confdir/../src
    local tmpdir=/tmp

    # Compare the md5 of the new metrics and old metrics' files, do not to
    # replace them if they are same, it will reduce the compiling time
    $LANGUAGE_PATH/bin/skull-config-gen.py -c $config \
        -h $tmpdir/config.h \
        -s $tmpdir/config.c

    # if the new config.x are different from the old ones, replace them
    if ! $(_compare_file $targetdir/config.h $tmpdir/config.h); then
        cp $tmpdir/config.h $targetdir/config.h
    fi

    if ! $(_compare_file $targetdir/config.c $tmpdir/config.c); then
        cp $tmpdir/config.c $targetdir/config.c
    fi
}

function action_c_gen_idl()
{
    # 1. generate protobuf-c source code for workflows
    (
        cd $SKULL_WORKFLOW_IDL_FOLDER
        for idl in ./*.proto; do
            protoc-c --c_out=$COMMON_FILE_LOCATION/src $idl
        done
    )

    # 2. generate user api source code
    local config=$1
    $LANGUAGE_PATH/bin/skull-idl-gen.py -c $config \
        -h $COMMON_FILE_LOCATION/src/skull_txn_sharedata.h \
        -s $COMMON_FILE_LOCATION/src/skull_txn_sharedata.c
}
