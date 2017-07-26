# This is the utility functions for skull build, which can be easiliy to build
# the entire project without must move to the top folder, meanwhile we can
# build the project in any where inside a valid skull project
#
# NOTES: This is included by the main script `skull`

# For build actions, we will do it as much as careful
#set -e

function action_build()
{
    # Move to the top level of the project to do the build
    # NOTES: since we pass through the user input args to make, so meanwhile
    # all the args will be passed to the main Makefile(e.g. 'CC=clang').
    cd $SKULL_PROJ_ROOT

    # Check whether it is the clean action
    local clean=false
    local args=($@)
    for arg in ${args[@]}; do
        if [ "$arg" = "clean" ]; then
            clean=true;
            break;
        fi
    done

    # Build accoring to the args
    if $clean; then
        _action_clean $@
    else
        _action_prepare
        _action_build $@
    fi
}

function action_build_usage()
{
    echo "Usage:"
    echo "  skull build [arg(s)...]"
    echo ""
    echo "Args:"
    echo "  - check"
    echo "  - valgrind-check"
    echo "  - clean"
    echo ""
    echo "Example:"
    echo "  skull buld"
    echo "  skull buld CC=clang"
    echo "  skull buld check"
    echo "  skull buld valgrind-check"
}

function _action_prepare()
{
    echo "Generating counters..."
    action_common --metrics-gen || exit 1

    echo "Generating transaction/service IDLs..."
    action_common --idl-gen || exit 1

    echo "Generating configs..."
    action_common --config-gen || exit 1
}

function _action_build()
{
    echo "Building modules/services..."
    exec make $@
}

function _action_clean()
{
    echo "Cleaning ..."
    exec make $@
}
