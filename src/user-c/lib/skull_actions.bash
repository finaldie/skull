# This is the bash script for C language
# NOTES: This script must implement the following functions:
#  - `action_$lang_module_add`
#  - `action_$lang_common_create`
#  - `action_$lang_gen_metrics`
#
# NOTES2: before running any methods in file, skull will change the current
# folder to the top of the project `SKULL_PROJECT_ROOT`, so it's no need to
# change it again manually

LANGUAGE_PATH=share/skull/lang/c

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
    cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.inc.tpl $SKULL_PROJ_ROOT/.Makefile.inc.c
    cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.targets.tpl $SKULL_PROJ_ROOT/.Makefile.targets.c
    cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.common.targets.tpl $SKULL_PROJ_ROOT/.Makefile.common.targets.c

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
        cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.common.tpl $COMMON_FILE_LOCATION/Makefile
    fi
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
