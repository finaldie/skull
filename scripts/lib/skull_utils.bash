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
    local module_lang=$(_module_language $module)

    if [ -z "$module" ]; then
        echo "Error: not in a module, please switch to a module folder first" >&2
        return 1
    fi

    if [ -z "$module_lang" ]; then
        echo "Error: unsupported module language type" >&2
        return 1
    fi

    _run_lang_action $module_lang $action $@
}
