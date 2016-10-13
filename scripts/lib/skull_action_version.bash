# This is the utility functions for skull version, show users the version
# information
#
# NOTES: This is included by the main script `skull`

function action_version()
{
    local changelog=$SKULL_ROOT/etc/skull/ChangeLog.md
    local build_date=`cat $changelog | head -1 | awk '{print $2}'`
    local version=`cat $changelog | head -1 | awk '{print $3}'`

    echo -n "Skull $version $build_date - A fast to start, easy to maintain, "
    echo "high productivity serving framework."
    echo "More information see https://github.com/finaldie/skull"
}

function action_version_usage()
{
    echo "usage:"
    echo "  skull version"
}
