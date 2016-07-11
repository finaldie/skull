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
LANGUAGE_PATH=$SKULL_ROOT/share/skull/lang/cpp
COMMON_FILE_LOCATION=$SKULL_PROJ_ROOT/src/common/cpp
PROTO_FOLDER_NAME=protos
PROTO_FOLDER=$COMMON_FILE_LOCATION/src/$PROTO_FOLDER_NAME

# return:
#  - 0 if this is a valid C module
#  - 1 if this is not a valid C module
function action_cpp_module_valid()
{
    # for now, just check whether there is mod.c exist
    local module=$1
    if [ -f $SKULL_PROJ_ROOT/src/modules/$module/src/module.cpp ]; then
        return 0
    else
        return 1
    fi
}

function action_cpp_module_add()
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

    cp $LANGUAGE_PATH/etc/config.yaml         $module_path/config/config.yaml
    cp $LANGUAGE_PATH/etc/test_config.yaml    $module_path/tests/test_config.yaml
    cp $LANGUAGE_PATH/share/module.cpp        $module_path/src/module.cpp
    cp $LANGUAGE_PATH/share/test_module.cpp   $module_path/tests/test_module.cpp
    cp $LANGUAGE_PATH/share/gitignore-module  $module_path/.gitignore
    cp $LANGUAGE_PATH/share/ycm_extra_conf.py $module_path/.ycm_extra_conf.py

    sed -i "s/{MODULE_NAME}/$module/g" $module_path/src/module.cpp
    sed -i "s/{MODULE_NAME}/$module/g" $module_path/tests/test_module.cpp

    # copy makefile templates
    cp $LANGUAGE_PATH/share/Makefile.mod     $module_path/Makefile
    cp $LANGUAGE_PATH/share/Makefile.inc     $SKULL_MAKEFILE_FOLDER/Makefile.cpp.inc
    cp $LANGUAGE_PATH/share/Makefile.targets $SKULL_MAKEFILE_FOLDER/Makefile.cpp.targets

    # generate a static config code for user
    local module_config=$module_path/config/config.yaml
    action_cpp_gen_config $module_config

    return 0
}

function action_cpp_common_create()
{
    if [ -d "$COMMON_FILE_LOCATION" ]; then
        echo "notice: the common/cpp folder has already exist, ignore it"
        return 0
    fi

    # create common folders
    mkdir -p $COMMON_FILE_LOCATION/src
    mkdir -p $COMMON_FILE_LOCATION/tests
    mkdir -p $COMMON_FILE_LOCATION/lib

    # move the Makefile to common/cpp only when there is no Makefile in there
    if [ ! -f $COMMON_FILE_LOCATION/Makefile ]; then
        cp $LANGUAGE_PATH/share/Makefile.common \
            $COMMON_FILE_LOCATION/Makefile
    fi

    # copy gitignore
    if [ ! -f $COMMON_FILE_LOCATION/.gitignore ]; then
        cp $LANGUAGE_PATH/share/gitignore-common \
            $COMMON_FILE_LOCATION/.gitignore
    fi

    # copy common makefile targets
    cp $LANGUAGE_PATH/share/Makefile.common.targets \
        $SKULL_MAKEFILE_FOLDER/Makefile.common.cpp.targets

    # generate the metrics
    action_cpp_gen_metrics $SKULL_METRICS_FILE

    # generate idl source code according to the idls
    local config=$SKULL_PROJ_ROOT/config/skull-config.yaml
    action_cpp_gen_idl $config

    # copy the unit test file
    cp $LANGUAGE_PATH/share/test_common.cpp \
        $COMMON_FILE_LOCATION/tests/test_common.cpp
}

function action_cpp_gen_metrics()
{
    local config=$1
    local tmpdir=/tmp
    local tmp_header_file=$tmpdir/skull_metrics.h
    local tmp_source_file=$tmpdir/skull_metrics.cpp
    local header_file=$COMMON_FILE_LOCATION/src/skull_metrics.h
    local source_file=$COMMON_FILE_LOCATION/src/skull_metrics.cpp

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

function action_cpp_gen_config()
{
    local config=$1
    local confdir=`dirname $config`
    local targetdir=$confdir/../src
    local tmpdir=/tmp

    # Compare the md5 of the new metrics and old metrics' files, do not to
    # replace them if they are same, it will reduce the compiling time
    $LANGUAGE_PATH/bin/skull-config-gen.py -c $config \
        -h $tmpdir/config.h

    # if the new config.x are different from the old ones, replace them
    if ! $(_compare_file $targetdir/config.h $tmpdir/config.h); then
        cp $tmpdir/config.h $targetdir/config.h
    fi
}

function action_cpp_gen_idl()
{
    # 1. generate protobuf source code for workflows
    (
        cd $SKULL_WORKFLOW_IDL_FOLDER
        mkdir -p $PROTO_FOLDER
        local proto_cnt=`ls ./ | grep ".proto" | wc -l`
        if [ $proto_cnt -eq 0 ]; then
            return 0
        fi

        for idl in ./*.proto; do
            protoc --cpp_out=$PROTO_FOLDER $idl
        done
    )

    # 2. generate user api source code
    _generate_protos
    return 0
}

# Service Related
function action_cpp_service_valid()
{
    local service=$1
    if [ -f $SKULL_PROJ_ROOT/src/services/$service/src/service.cpp ]; then
        return 0
    else
        return 1
    fi
}

function action_cpp_service_add()
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

    cp $LANGUAGE_PATH/etc/config.yaml         $srv_path/config/config.yaml
    cp $LANGUAGE_PATH/etc/test_config.yaml    $srv_path/tests/test_config.yaml
    cp $LANGUAGE_PATH/share/service.cpp       $srv_path/src/service.cpp
    cp $LANGUAGE_PATH/share/test_service.cpp  $srv_path/tests/test_service.cpp
    cp $LANGUAGE_PATH/share/gitignore-service $srv_path/.gitignore
    cp $LANGUAGE_PATH/share/ycm_extra_conf.py $srv_path/.ycm_extra_conf.py

    sed -i "s/{SERVICE_NAME}/$service/g" $srv_path/src/service.cpp
    sed -i "s/{SERVICE_NAME}/$service/g" $srv_path/tests/test_service.cpp

    # copy makefile templates
    cp $LANGUAGE_PATH/share/Makefile.svc      $srv_path/Makefile
    cp $LANGUAGE_PATH/share/Makefile.inc      $SKULL_MAKEFILE_FOLDER/Makefile.cpp.inc
    cp $LANGUAGE_PATH/share/Makefile.targets  $SKULL_MAKEFILE_FOLDER/Makefile.cpp.targets

    # generate a static config code for user
    local srv_config=$srv_path/config/config.yaml
    action_cpp_gen_config $srv_config

    return 0
}

function _generate_protos()
{
    local wf_proto_raw_list=`ls -1 $SKULL_WORKFLOW_IDL_FOLDER | grep .proto`
    local wf_proto_list=`echo $wf_proto_raw_list | sed 's/ /|/g'`

    local svc_proto_raw_list=`ls -1 $SKULL_SERVICE_IDL_FOLDER | grep .proto`
    local svc_proto_list=`echo $svc_proto_raw_list | sed 's/ /|/g'`

    local param_list=""
    if [ -n "$wf_proto_list" ]; then
        param_list+="-w $wf_proto_list"
    fi

    if [ -n "$svc_proto_list" ]; then
        param_list+=" -s $svc_proto_list"
    fi

    $LANGUAGE_PATH/bin/skull-idl-gen.py -p $PROTO_FOLDER_NAME \
        -o $COMMON_FILE_LOCATION/src/skull_protos.h \
        $param_list
}

function action_cpp_service_api_gen()
{
    # 1. generate protobuf source code for workflows
    (
        cd $SKULL_SERVICE_IDL_FOLDER
        mkdir -p $PROTO_FOLDER

        for idl in ./*.proto; do
            protoc --cpp_out=$PROTO_FOLDER $idl
        done
    )

    # 2. generate idls
    _generate_protos
    return 0
}
