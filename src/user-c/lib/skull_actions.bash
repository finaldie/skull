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

LANGUAGE_PATH=share/skull/lang/c

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
    if [ -d $SKULL_PROJ_ROOT/src/modules/$module ]; then
        echo "Notice: the module [$module] has already exist"
        return 1
    fi

    # add module folder and files
    mkdir -p $SKULL_PROJ_ROOT/src/modules/$module/src
    mkdir -p $SKULL_PROJ_ROOT/src/modules/$module/tests
    mkdir -p $SKULL_PROJ_ROOT/src/modules/$module/config

    cp $SKULL_ROOT/$LANGUAGE_PATH/share/mod.c.tpl $SKULL_PROJ_ROOT/src/modules/$module/src/mod.c
    cp $SKULL_ROOT/$LANGUAGE_PATH/share/config.yaml.tpl $SKULL_PROJ_ROOT/src/modules/$module/config/config.yaml

    # copy makefiel templates
    cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.tpl $SKULL_PROJ_ROOT/src/modules/$module/Makefile
    cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.inc.tpl $SKULL_MAKEFILE_FOLDER/Makefile.c.inc
    cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.targets.tpl $SKULL_MAKEFILE_FOLDER/Makefile.c.targets

    # convert config to code
    local module_config=$SKULL_PROJ_ROOT/src/modules/$module/config/config.yaml
    action_c_gen_config $module_config

    return 0
}

COMMON_FILE_LOCATION=$SKULL_PROJ_ROOT/src/common/c

function action_c_common_create()
{
    if [ -d "$COMMON_FILE_LOCATION" ]; then
        echo "notice: the common/c folder has already exist, ignore it"
        return 0
    fi

    # create common folers
    mkdir -p $COMMON_FILE_LOCATION/src
    mkdir -p $COMMON_FILE_LOCATION/tests

    # move the Makefile to common/c only when there is no Makefile in common/c
    if [ ! -f $COMMON_FILE_LOCATION/Makefile ]; then
        cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.common.tpl \
            $COMMON_FILE_LOCATION/Makefile
    fi

    cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.common.targets.tpl \
        $SKULL_MAKEFILE_FOLDER/Makefile.common.c.targets

    # generate the metrics
    action_c_gen_metrics $SKULL_METRICS_FILE

    # generate idl source code according to the idls
    local config=$SKULL_PROJ_ROOT/config/skull-config.yaml
    action_c_gen_idl $config
}

function action_c_gen_metrics()
{
    local config=$1

    # TODO: compare the md5 of the new metrics and old metrics' files, do not to
    # replace them if they are same, it will reduce the compiling time
    $SKULL_ROOT/$LANGUAGE_PATH/bin/skull-metrics-gen.py -c $config \
        -h $COMMON_FILE_LOCATION/src/skull_metrics.h \
        -s $COMMON_FILE_LOCATION/src/skull_metrics.c
}

function action_c_gen_config()
{
    local config=$1
    local confdir=`dirname $config`
    local targetdir=$confdir/../src
    local tmpdir=/tmp

    # TODO: compare the md5 of the new metrics and old metrics' files, do not to
    # replace them if they are same, it will reduce the compiling time
    $SKULL_ROOT/$LANGUAGE_PATH/bin/skull-config-gen.py -c $config \
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
    # 1. generate protobuf-c source code
    (
        cd $SKULL_PROJ_ROOT/config
        for idl in ./*.proto; do
            protoc-c --c_out=$SKULL_PROJ_ROOT/src/common/c/src $idl
        done
    )

    # 2. generate user api source code
    local config=$1
    $SKULL_ROOT/$LANGUAGE_PATH/bin/skull-idl-gen.py -c $config \
        -h $SKULL_PROJ_ROOT/src/common/c/src/skull_idl.h \
        -s $SKULL_PROJ_ROOT/src/common/c/src/skull_idl.c
}
