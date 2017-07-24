#!/bin/sh

##
# This script is used for getting compile time static variables
#

FINAL_MACROS=""

# based version
version=`cat ../ChangeLog.md | head -1 | awk '{print $3}'`
FINAL_MACROS="$FINAL_MACROS -DSKULL_VERSION=\\\"$version\\\" "

# git sha1
git_sha1=`git log -1 | head -1 | awk '{print $2}'`
if [ $? -eq 0 ]; then
    FINAL_MACROS="$FINAL_MACROS -DSKULL_GIT_SHA1=\\\"${git_sha1}\\\" "
else
    FINAL_MACROS="$FINAL_MACROS -DSKULL_GIT_SHA1=\\\"unknown\\\" "
fi

# compiler version (fetch it from builtin macros)

echo "$FINAL_MACROS"

