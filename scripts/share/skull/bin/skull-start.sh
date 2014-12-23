#!/bin/sh

if [ $# = 0 ]; then
    echo "Fatal: Missing skull config"
    exit 1
fi

# 1. Put some custom settings/actions here...

# 2. Start skull engine
skull_config=$1
export LD_LIBRARY_PATH=`pwd`/common/c

skull-engine -c $skull_config
