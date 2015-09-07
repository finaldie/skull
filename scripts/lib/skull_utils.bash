# This is the utility functions for skull actions:
#
# NOTES: This is included by the main script `skull`

function get_proj_root()
{
    local current_dir=`pwd`

    if [ -d .skull ]; then
        SKULL_PROJ_ROOT=$current_dir
        return 0
    fi

    # If doesn't find the .skull in current folder, find its parent folder
    current_dir=`dirname $current_dir`
    while [ ! -d $current_dir/.skull ]; do
        if [ $current_dir = "/" ]; then
            return 1
        else
            current_dir=`dirname $current_dir`
        fi
    done

    SKULL_PROJ_ROOT=$current_dir
    return 0
}

function _preload_language_actions()
{
    # Load Language action scripts, for now, we only have C language
    for lang_dir in $SKULL_ROOT/share/skull/lang/*;
    do
        if [ -d $lang_dir ]; then
            . $lang_dir/lib/skull_actions.bash
        fi
    done
}

function _get_language_list()
{
    local langs
    local idx=0

    for lang_dir in $SKULL_ROOT/share/skull/lang/*;
    do
        if [ -d $lang_dir ]; then
            local lang_name=`basename $lang_dir`
            langs[$idx]=$lang_name
            idx=`expr $idx + 1`
        fi
    done

    echo ${langs[*]}
}

function _check_language()
{
    local langs=$1
    local input_lang=$2

    for lang in "$langs"; do
        if [ "$lang" = "$input_lang" ]; then
            return 0
        fi
    done

    echo "Fatal: module add failed, input the correct language" >&2
    return 1
}

# return:
#  - 0 if the two files are the same
#  - 1 if the two files are not the same
function _compare_file()
{
    local file1=$1
    local file2=$2

    if [ ! -f $file1 ]; then
        return 1
    fi

    if [ -f $file1 ] && [ ! -f $file2 ]; then
        return 1
    fi

    if [ -f $file1 ] && [ -f $file2 ]; then
        local md5_file1=`md5sum $file1 | awk '{print $1}'`
        local md5_file2=`md5sum $file2 | awk '{print $2}'`

        if [ $md5_file1 == $md5_file2 ]; then
            return 0
        else
            return 1
        fi
    fi
}

# 'remove' action is very dangerous, so this function is wrap the 'rm' and
#  do it when everything is correct
function _safe_rm()
{
    if [ $# = 0 ]; then
        return 0
    fi

    local removed_target=($@)

    for target in ${removed_target[*]}; do
        local absolute_path=`readlink -f $target`

        # check the path is part of SKULL_PROJ_ROOT
        echo "$absolute_path" | grep "$target"
        if [ $? -eq 1 ]; then
            echo "$absolute_path cannot be deleted" >&2
        else
            rm -f $absolute_path
        fi
    done
}

# return current module name if user indeed inside a module folder
function _current_module()
{
    local module_path=`echo "$SKULL_OLD_LOACTION" | grep -oP "src/modules/(\w+)"`
    local module=""
    if [ ! -z "$module_path" ]; then
        module=`basename $module_path`
    fi

    echo "$module"
}

function _current_module_config()
{
    local module=$1
    local module_config=$SKULL_PROJ_ROOT/src/modules/$module/config/config.yaml
    echo $module_config
}

# param module_name
# return
#  - language name if this is a supported language. for exmaple: c
#  - empty if this is a unsupported language
function _module_language()
{
    local module=$1
    local langs=$(_get_language_list)

    for lang in "$langs"; do
        if $(_run_lang_action $lang $SKULL_LANG_MODULE_VALID $module); then
            echo $lang
        fi
    done

    echo ""
}

# param action_name
# param language_name
# return the output of the corresponding output of that action
function _run_lang_action()
{
    local lang=$1
    local action=$2
    shift 2

    action_${lang}_$action $@
}

# param action_name
# return the output of the corresponding output of that action
function _run_module_action()
{
    local action=$1
    shift

    local module=$(_current_module)
    if [ -z "$module" ]; then
        echo "Error: not in a module, please switch to a module folder first" >&2
        return 1
    fi

    local module_lang=$(_module_language $module)
    if [ -z "$module_lang" ]; then
        echo "Error: unsupported module language type" >&2
        return 1
    fi

    _run_lang_action $module_lang $action $@
}

# verify a name must be in [0-9a-zA-Z_]
function _check_name()
{
    local name=$1

    if [ -z "$name" ]; then
        echo "Error: name must not be empty" >&2
        return 1
    fi

    local ret=`echo "$name" | grep -P "[^\w]" | wc -l`
    if [ "$ret" = "1" ]; then
        echo "Error: name must be [0-9a-zA-Z_]" >&2
        return 1
    fi

    return 0
}

################ Service related ###############

# return current module name if user indeed inside a module folder
function _current_service()
{
    local srv_path=`echo "$SKULL_OLD_LOACTION" | grep -oP "src/services/(\w+)"`
    local service=""
    if [ ! -z "$srv_path" ]; then
        service=`basename $srv_path`
    fi

    echo "$service"
}

function _current_service_config()
{
    local service=$1
    local srv_config=$SKULL_PROJ_ROOT/src/services/$service/config/config.yaml
    echo $srv_config
}

# param module_name
# return
#  - language name if this is a supported language. for exmaple: c
#  - empty if this is a unsupported language
function _service_language()
{
    local service=$1
    local langs=$(_get_language_list)

    for lang in "$langs"; do
        if $(_run_lang_action $lang $SKULL_LANG_SERVICE_VALID $service); then
            echo $lang
        fi
    done

    echo ""
}

# param action_name
# return the output of the corresponding output of that action
function _run_service_action()
{
    local action=$1
    shift

    local service=$(_current_service)
    if [ -z "$service" ]; then
        echo "Error: not in a service, please switch to a service folder first" >&2
        return 1
    fi

    local service_lang=$(_service_language $service)
    if [ -z "$service_lang" ]; then
        echo "Error: unsupported service language type [$service_lang]" >&2
        return 1
    fi

    _run_lang_action $service_lang $action $@
}

function _utils_srv_api_copy()
{
    local service=$1
    local srv_idl_folder=$SKULL_PROJ_ROOT/src/services/$service/idl
    local srv_api_files=`ls $srv_idl_folder | grep ".proto"`

    local srv_api_cnt=`echo $srv_api_files | wc -w`
    if [ $srv_api_cnt -eq 0 ]; then
        return 0
    fi

    # copy the api proto files to topdir/idls/service
    ls -1 $srv_idl_folder | grep .proto | while read api_file; do
        cp $srv_idl_folder/$api_file $SKULL_SERVICE_IDL_FOLDER

        echo "$api_file"
    done
}

# generate service idls (apis)
function skull_utils_srv_api_gen()
{
    local service_list=`ls $SKULL_PROJ_ROOT/src/services`
    local api_list=""

    # copy all service api protos to idls/service and get api list
    for service in "$service_list"; do
        local apis=$(_utils_srv_api_copy $service)

        api_list+=$apis
        api_list+=" "
    done

    # check api cnt
    local api_cnt=`echo $api_list | wc -w`
    if [ $api_cnt -eq 0 ]; then
        return 0
    fi

    # convert api protos to target language source files
    local langs=$(_get_language_list)

    for lang in "$langs"; do
        _run_lang_action $lang $SKULL_LANG_SERVICE_API_GEN $api_list
    done

    return 0
}
