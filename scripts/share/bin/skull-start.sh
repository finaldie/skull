#!/bin/sh

if [ $# = 0 ]; then
    echo "Fatal: Missing skull config"
    exit 1
fi

# 1. Put some custom settings/actions here...

# 2. Rrepare the environment variables
skull_config=$1
skull_rundir=`dirname $skull_config`
export LD_LIBRARY_PATH=$skull_rundir/lib

# 3. Start skull engine
skull-engine -c $skull_config
