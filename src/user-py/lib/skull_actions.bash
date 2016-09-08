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
#
# NOTES2: before running any methods in file, skull will change the current
# folder to the top of the project `SKULL_PROJECT_ROOT`, so it's no need to
# change it again manually

# static env var
LANGUAGE_PATH=$SKULL_ROOT/share/skull/lang/py
COMMON_FILE_LOCATION=$SKULL_PROJ_ROOT/src/common/py
PROTO_FOLDER_NAME=protos
PROTO_FOLDER=$COMMON_FILE_LOCATION/$PROTO_FOLDER_NAME

# return:
#  - 0 if this is a valid C module
#  - 1 if this is not a valid C module
function action_py_module_valid()
{
    # for now, just check whether there is mod.c exist
    local module=$1
    if [ -f $SKULL_PROJ_ROOT/src/modules/$module/src/module.py ]; then
        return 0
    else
        return 1
    fi
}

function action_py_module_add()
{
    local module=$1
    local module_path=$SKULL_PROJ_ROOT/src/modules/$module

    if [ -d "$module_path" ]; then
        echo "Notice: The module [$module] has already exist"
        return 0
    fi

    # add module folder and files
    mkdir -p $module_path

    cp $LANGUAGE_PATH/etc/config.yaml         $module_path/config.yaml
    cp $LANGUAGE_PATH/share/module.py         $module_path/module.py
    cp $LANGUAGE_PATH/share/__init__.py       $module_path/__init__.py
    cp $LANGUAGE_PATH/share/gitignore-module  $module_path/.gitignore

    sed -i "s/{MODULE_NAME}/$module/g" $module_path/module.py

    # copy makefile templates
    cp $LANGUAGE_PATH/share/Makefile.mod         $module_path/Makefile
    cp $LANGUAGE_PATH/share/Makefile.mod.targets $SKULL_MAKEFILE_FOLDER/Makefile.mod.py.targets

    # generate a static config code for user
    local module_config=$module_path/config.yaml
    action_cpp_gen_config $module_config

    return 0
}

function action_py_common_create()
{
    if [ -d "$COMMON_FILE_LOCATION" ]; then
        echo "notice: the common/cpp folder has already exist, ignore it"
        return 0
    fi

    # create common folders
    mkdir -p $COMMON_FILE_LOCATION

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
        $SKULL_MAKEFILE_FOLDER/Makefile.common.py.targets

    # generate the metrics
    action_py_gen_metrics $SKULL_METRICS_FILE

    # generate idl source code according to the idls
    local config=$SKULL_PROJ_ROOT/config/skull-config.yaml
    action_py_gen_idl $config
}

function action_py_gen_metrics()
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

    if ! $(sk_util_compare_file $tmp_header_file $header_file); then
        cp $tmp_header_file $header_file
    fi

    if ! $(sk_util_compare_file $tmp_source_file $source_file); then
        cp $tmp_source_file $source_file
    fi
}

function action_py_gen_config()
{
    local config=$1
    local confdir=`dirname $config`
    local targetdir=$confdir
    local tmpdir=/tmp

    ## Compare the md5 of the new metrics and old metrics' files, do not to
    ## replace them if they are same, it will reduce the compiling time
    #$LANGUAGE_PATH/bin/skull-config-gen.py -c $config \
    #    -h $tmpdir/config.h

    ## if the new config.x are different from the old ones, replace them
    #if ! $(sk_util_compare_file $targetdir/config.h $tmpdir/config.h); then
    #    cp $tmpdir/config.h $targetdir/config.h
    #fi
}

function action_py_gen_idl()
{
    # 1. generate protobuf source code for workflows/services
    (
        # 1. prepare building folder
        mkdir -p $PROTO_FOLDER
        cd $PROTO_FOLDER
        touch __init__.py

        # 2. copy workflow/service idls into building folder
        # 2.1 copy workflow idls
        local workflow_idls=($(sk_util_workflow_all_idls))
        for idl in ${workflow_idls[@]}; do
            cp $idl $PROTO_FOLDER
        done

        # 2.2 copy service idls
        local service_idls=($(sk_util_service_all_idls))
        for idl in ${service_idls[@]}; do
            cp $idl $PROTO_FOLDER
        done

        # 2.3 check at least have one proto to build
        local proto_cnt=`ls ./ | grep ".proto" | wc -l`
        if [ $proto_cnt -eq 0 ]; then
            return 0
        fi

        # 2.4 compile proto files to python source files
        for idl in ./*.proto; do
            protoc --python_out=$PROTO_FOLDER $idl
        done
    )

    # 2. generate user api (header file)
    _generate_py_protos
    return 0
}

# Service Related
function action_py_service_valid()
{
    # Do not support service so far
    return 1
}

function action_py_service_add()
{
    # Do not support service so far
    return 0
}

function _generate_py_protos()
{
    local wf_proto_raw_list="$(sk_util_workflow_all_idls)"
    local wf_proto_list=`echo $wf_proto_raw_list | sed 's/ /|/g'`

    local svc_proto_raw_list="$(sk_util_service_all_idls)"
    local svc_proto_list=`echo $svc_proto_raw_list | sed 's/ /|/g'`

    local param_list=""
    if [ -n "$wf_proto_list" ]; then
        param_list+="-w $wf_proto_list"
    fi

    if [ -n "$svc_proto_list" ]; then
        param_list+=" -s $svc_proto_list"
    fi

    $LANGUAGE_PATH/bin/skull-idl-gen.py -p $PROTO_FOLDER_NAME \
        -o $COMMON_FILE_LOCATION/skull_protos.py \
        $param_list
}

