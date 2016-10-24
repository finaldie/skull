# This is the utility functions for skull actions:
#
# NOTES: This is included by the main script `skull`

function sk_util_get_proj_root()
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

function sk_util_yn_valid()
{
    if [ $# = 0 ]; then
        return 1
    fi

    local yn_list=(y Y n N)
    local input="$1"

    for ca in ${yn_list[@]}; do
        if [ "$ca" = "$input" ]; then
            return 0
        fi
    done

    return 1
}

function sk_util_yn_yes()
{
    if [ $# = 0 ]; then
        return 1
    fi

    local y_list=(y Y)
    local input="$1"

    for ca in ${y_list[@]}; do
        if [ "$ca" = "$input" ]; then
            return 0
        fi
    done

    return 1
}

function sk_util_preload_language_actions()
{
    # Load Language action scripts, for now, we only have C language
    for lang_dir in $SKULL_ROOT/share/skull/lang/*; do
        if [ -d "$lang_dir" ]; then
            . $lang_dir/lib/skull_actions.bash
        fi
    done
}

function sk_util_get_language_list()
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

function sk_util_check_language()
{
    local langs=($(sk_util_get_language_list))
    local input_lang=$1

    for lang in "${langs[@]}"; do
        if [ "$lang" = "$input_lang" ]; then
            return 0
        fi
    done

    echo "Fatal: module add failed, input the correct language" >&2
    return 1
}

function sk_util_is_number()
{
    local value="$1"
    local re="^[0-9]+$"

    if [[ $value =~ $re ]]; then
        return 0
    else
        return 1
    fi
}

# return:
#  - 0 if the two files are the same
#  - 1 if the two files are not the same
function sk_util_compare_file()
{
    local file1=$1
    local file2=$2

    if [ ! -f "$file1" ]; then
        return 1
    fi

    if [ ! -f "$file2" ]; then
        return 1
    fi

    local md5_file1=`md5sum "$file1" | awk '{print $1}'`
    local md5_file2=`md5sum "$file2" | awk '{print $1}'`

    if [ $md5_file1 == $md5_file2 ]; then
        return 0
    else
        return 1
    fi
}

# 'remove' action is very dangerous, so this function is wrap the 'rm' and
#  do it when everything is correct
function sk_util_safe_rm()
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
            echo "Notice: $absolute_path cannot be deleted, please review it again and delete it if really needed" >&2
        else
            rm -f $absolute_path
        fi
    done
}

# return current module name if user indeed inside a module folder
function sk_util_current_module()
{
    local module_path=`echo "$SKULL_OLD_LOACTION" | grep -oP "src/modules/(\w+)"`
    local module=""
    if [ ! -z "$module_path" ]; then
        module=`basename $module_path`
    fi

    echo "$module"
}

function sk_util_current_module_config()
{
    local module=$1
    local module_config=$SKULL_PROJ_ROOT/src/modules/$module/config/config.yaml
    echo $module_config
}

# param module_name
# return
#  - language name if this is a supported language. for exmaple: c
#  - empty if this is a unsupported language
function sk_util_module_language()
{
    local module=$1
    local langs=($(sk_util_get_language_list))

    for lang in "${langs[@]}"; do
        if $(sk_util_run_lang_action $lang $SKULL_LANG_MODULE_VALID $module); then
            echo $lang
        fi
    done

    echo ""
}

function sk_util_module_list()
{
    ls -1 $SKULL_MODULE_BASE_FOLDER;
}

# param action_name
# param language_name
# return the output of the corresponding output of that action
function sk_util_run_lang_action()
{
    local lang=$1
    local action=$2
    shift 2

    action_${lang}_$action $@
}

# param action_name
# return the output of the corresponding output of that action
function sk_util_run_module_action()
{
    local module=$1
    local action=$2
    shift 2

    local module_lang=$(sk_util_module_language $module)
    if [ -z "$module_lang" ]; then
        echo "Error: unsupported module language type" >&2
        return 1
    fi

    sk_util_run_lang_action $module_lang $action $@
}

# Generate module config code files
function sk_util_module_config_gen()
{
    local module=$1
    shift 1

    local module_config=$(sk_util_current_module_config $module)
    sk_util_run_module_action $module $SKULL_LANG_GEN_CONFIG $module_config
}

# verify a name must be in [0-9a-zA-Z_]
function sk_util_check_name()
{
    local name=$1

    if [ -z "$name" ]; then
        echo "Error: name must not be empty" >&2
        return 1
    fi

    local ret=`echo "$name" | grep "[^0-9a-zA-Z_]" | wc -l`
    if [ "$ret" = "1" ]; then
        echo "Error: name must be [0-9a-zA-Z_]" >&2
        return 1
    fi

    return 0
}

################ Service related ###############

# return current module name if user indeed inside a module folder
function sk_util_current_service()
{
    local srv_path=`echo "$SKULL_OLD_LOACTION" | grep -oP "src/services/(\w+)"`
    local service=""
    if [ ! -z "$srv_path" ]; then
        service=`basename $srv_path`
    fi

    echo "$service"
}

function sk_util_current_service_config()
{
    local service=$1
    local srv_config=$SKULL_PROJ_ROOT/src/services/$service/config/config.yaml
    echo $srv_config
}

# param module_name
# return
#  - language name if this is a supported language. for exmaple: c
#  - empty if this is a unsupported language
function sk_util_service_language()
{
    local service=$1
    local langs=($(sk_util_get_language_list))

    for lang in "${langs[@]}"; do
        if $(sk_util_run_lang_action $lang $SKULL_LANG_SERVICE_VALID $service); then
            echo $lang
        fi
    done

    echo ""
}

function sk_util_service_list()
{
    ls -1 $SKULL_SERVICE_BASE_FOLDER;
}

function sk_util_service_all_idls()
{
    local service_list=($(sk_util_service_list))

    for service in ${service_list[@]}; do
        local idls=(`ls -1 $SKULL_SERVICE_BASE_FOLDER/$service/idl/ | grep ".proto"`)

        for idl in ${idls[@]}; do
            echo "$SKULL_SERVICE_BASE_FOLDER/$service/idl/$idl"
        done
    done
}

function sk_util_workflow_all_idls()
{
    local idls=(`ls -1 $SKULL_WORKFLOW_IDL_FOLDER | grep ".proto"`)

    for idl in ${idls[@]}; do
        echo $SKULL_WORKFLOW_IDL_FOLDER/$idl
    done
}

# param action_name
# return the output of the corresponding output of that action
function sk_util_run_service_action()
{
    local service=$1
    local action=$2
    shift 2

    local service_lang=$(sk_util_service_language $service)
    if [ -z "$service_lang" ]; then
        echo "Error: unsupported service language type [$service_lang]" >&2
        return 1
    fi

    sk_util_run_lang_action $service_lang $action $@
}

# Generate service config code files
function sk_util_service_config_gen()
{
    local service=$1
    shift 1

    local svc_config=$(sk_util_current_service_config $service)
    sk_util_run_service_action $service $SKULL_LANG_GEN_CONFIG $svc_config
}

