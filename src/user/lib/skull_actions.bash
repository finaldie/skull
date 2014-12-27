# This is the bash script for C language
# NOTES: This script must implement the following functions:
#  - `action_$lang_add`
#  - `action_$lang_gen_metrics`
#
# NOTES2: before running any methods in file, skull will change the current
# folder to the top of the project `SKULL_PROJECT_ROOT`, so it's no need to
# change it again manually

LANGUAGE_PATH=share/skull/lang/c

function action_c_add()
{
    local module=$1
    if [ -d src/modules/$module ]; then
        echo "Notice: the module [$module] has already exist"
        return 1
    fi

    # add module folder and files
    mkdir -p src/modules/$module/src
    mkdir -p src/modules/$module/tests
    mkdir -p src/modules/$module/config

    cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.tpl src/modules/$module/Makefile
    cp $SKULL_ROOT/$LANGUAGE_PATH/share/mod.c.tpl src/modules/$module/src/mod.c
    cp $SKULL_ROOT/$LANGUAGE_PATH/share/config.yaml.tpl src/modules/$module/config/config.yaml

    return 0
}

COMMON_FILE_LOCATION=src/common/c

function action_c_gen_metrics()
{
    local config=$1

    # TODO: compare the md5 of the new metrics and old metrics' files, do not to
    # replace them if they are same, it will reduce the compiling time
    mkdir -p $COMMON_FILE_LOCATION/src
    mkdir -p $COMMON_FILE_LOCATION/tests

    $SKULL_ROOT/$LANGUAGE_PATH/bin/skull-metrics-gen.py -c $config \
        -h $COMMON_FILE_LOCATION/src/skull_metrics.h \
        -s $COMMON_FILE_LOCATION/src/skull_metrics.c

    # move the Makefile to common/c only when there is no Makefile in common/c
    if [ ! -f $COMMON_FILE_LOCATION/Makefile ]; then
        cp $SKULL_ROOT/$LANGUAGE_PATH/share/Makefile.common.tpl $COMMON_FILE_LOCATION/Makefile
    fi
}
