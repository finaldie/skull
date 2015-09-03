# This is the utility functions for skull build, which can be easiliy to build
# the entire project without must move to the top folder, meanwhile you can
# build the project any where when you in a skull project
#
# NOTES: This is included by the main script `skull`

# For build actions, we will do it as much as careful
set -e

function action_build()
{
    # Fork and move to the top level of the project to do the build
    # NOTES: since we pass through the user input args to make, so meanwhile
    # all the args will be passed to the main Makefile(e.g. 'CC=clang').
    (
        cd $SKULL_PROJ_ROOT

        echo "generate metrics..."
        action_common --metrics-gen

        echo "generate transcation idls..."
        action_common --idl-gen

        echo "generate service apis..."
        action_common --srv-idl-gen

        echo "build modules/services..."
        make $@
    )
}

function action_build_usage()
{
    echo "usage:"
    echo "  skull build [arg(s)...]"
    echo ""
    echo "args:"
    echo "  - check"
    echo "  - valgrind-check"
    echo "  - clean"
    echo ""
    echo "example:"
    echo "  skull buld"
    echo "  skull buld CC=clang"
    echo "  skull buld check"
    echo "  skull buld valgrind-check"
}
