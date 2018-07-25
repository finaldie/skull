# This is the utility functions for skull version, show users the version
# information
#
# NOTES: This is included by the main script `skull`

function action_version()
{
    local version=$(sk_util_version)

    echo "Skull $version - A fast to start, easy to maintain," \
         "high productive serving framework."
    echo "More information see https://github.com/finaldie/skull"
}

function action_version_usage()
{
    echo "Usage:"
    echo "  skull version"
}
