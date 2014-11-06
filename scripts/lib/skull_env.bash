# This is the bash script for creating basic environment of skull scripts
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

