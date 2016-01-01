# This is the bash script for C language
# NOTES: This script must implement the following functions:
#  - `action_$lang_module_valid`
#  - `action_$lang_module_add`
#  - `action_$lang_common_create`
#  - `action_$lang_gen_metrics`
#  - `action_$lang_gen_config`
#  - `action_$lang_gen_idl`
#  - `action_$lang_service_valid`
#  - `action_$lang_service_add`
#  - `action_$lang_service_api_gen`
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
        return 0
    fi

    # add module folder and files
    mkdir -p $module_path/src
    mkdir -p $module_path/tests
    mkdir -p $module_path/config
    mkdir -p $module_path/lib

    cp $LANGUAGE_PATH/share/module.c         $module_path/src/module.c
    cp $LANGUAGE_PATH/etc/config.yaml        $module_path/config/config.yaml
    cp $LANGUAGE_PATH/share/test_module.c    $module_path/tests/test_module.c
    cp $LANGUAGE_PATH/etc/test_config.yaml   $module_path/tests/test_config.yaml
    cp $LANGUAGE_PATH/share/gitignore-module $module_path/.gitignore

    # copy makefile templates
    cp $LANGUAGE_PATH/share/Makefile.mod.tpl $module_path/Makefile
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
    cp $LANGUAGE_PATH/share/test_common.c \
        $COMMON_FILE_LOCATION/tests/test_common.c
}

function action_c_gen_metrics()
{
    local config=$1
    local tmpdir=/tmp
    local tmp_header_file=$tmpdir/skull_metrics.h
    local tmp_source_file=$tmpdir/skull_metrics.c
    local header_file=$COMMON_FILE_LOCATION/src/skull_metrics.h
    local source_file=$COMMON_FILE_LOCATION/src/skull_metrics.c

    $LANGUAGE_PATH/bin/skull-metrics-gen.py -c $config \
        -h $tmp_header_file \
        -s $tmp_source_file

    if ! $(_compare_file $tmp_header_file $header_file); then
        cp $tmp_header_file $header_file
    fi

    if ! $(_compare_file $tmp_source_file $source_file); then
        cp $tmp_source_file $source_file
    fi
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
        local proto_cnt=`ls ./ | grep ".proto" | wc -l`
        if [ $proto_cnt -eq 0 ]; then
            return 0
        fi

        for idl in ./*.proto; do
            protoc-c --c_out=$COMMON_FILE_LOCATION/src $idl
        done
    )

    # 2. generate user api source code
    local config=$1
    $LANGUAGE_PATH/bin/skull-idl-gen.py -m txn -c $config \
        -h $COMMON_FILE_LOCATION/src/skull_txn_sharedata.h \
        -s $COMMON_FILE_LOCATION/src/skull_txn_sharedata.c

    return 0
}

# Service Related
function action_c_service_valid()
{
    local service=$1
    if [ -f $SKULL_PROJ_ROOT/src/services/$service/src/service.c ]; then
        return 0
    else
        return 1
    fi
}

function action_c_service_add()
{
    local service=$1
    local srv_path=$SKULL_PROJ_ROOT/src/services/$service

    if [ -d "$srv_path" ]; then
        echo "Notice: the service [$service] has already exist"
        return 1
    fi

    # add module folder and files
    mkdir -p $srv_path/src
    mkdir -p $srv_path/tests
    mkdir -p $srv_path/config
    mkdir -p $srv_path/lib
    mkdir -p $srv_path/idl

    cp $LANGUAGE_PATH/share/service.c         $srv_path/src/service.c
    cp $LANGUAGE_PATH/etc/config.yaml         $srv_path/config/config.yaml
    cp $LANGUAGE_PATH/share/test_service.c    $srv_path/tests/test_service.c
    cp $LANGUAGE_PATH/etc/test_config.yaml    $srv_path/tests/test_config.yaml
    cp $LANGUAGE_PATH/share/gitignore-service $srv_path/.gitignore

    # copy makefile templates
    cp $LANGUAGE_PATH/share/Makefile.svc.tpl     $srv_path/Makefile
    cp $LANGUAGE_PATH/share/Makefile.inc.tpl     $SKULL_MAKEFILE_FOLDER/Makefile.c.inc
    cp $LANGUAGE_PATH/share/Makefile.targets.tpl $SKULL_MAKEFILE_FOLDER/Makefile.c.targets

    # generate a static config code for user
    local srv_config=$srv_path/config/config.yaml
    action_c_gen_config $srv_config

    return 0
}

function action_c_service_api_gen()
{
    local api_list=($@)

    # 1. generate protobuf-c source code for workflows
    (
        cd $SKULL_SERVICE_IDL_FOLDER

        for idl in ${api_list[*]}; do
            protoc-c --c_out=$COMMON_FILE_LOCATION/src $idl
        done
    )

    # 2. generate idls
    local api_name_list=""
    for api_file in ${api_list[*]}; do
        api_name_list+=`basename $api_file`
        api_name_list+="|"
    done

    $LANGUAGE_PATH/bin/skull-idl-gen.py -m service \
        -h $COMMON_FILE_LOCATION/src/skull_srv_api_proto.h \
        -s $COMMON_FILE_LOCATION/src/skull_srv_api_proto.c \
        -l $api_name_list

    return 0
}
