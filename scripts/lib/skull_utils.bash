# This is the utility functions for skull actions:
#
# NOTES: This is included by the main script `skull`

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
