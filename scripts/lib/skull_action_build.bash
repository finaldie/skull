# This is the utility functions for skull build, which can be easiliy to build
# the entire project without must move to the top folder, meanwhile you can
# build the project any where when you in a skull project
#
# NOTES: This is included by the main script `skull`

function action_build()
{
    # Fork and move to the top level of the project to do the build
    (
        cd $SKULL_PROJ_ROOT
        make
    )
}
