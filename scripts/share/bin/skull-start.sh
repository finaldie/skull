#!/bin/sh

set -e

##################################### Utils ####################################
usage() {
    echo "usage:"
    echo "  skull-start.sh -c config [--memcheck|--gdb|--strace]"
}

skull_start() {
    skull-engine -c $skull_config
}

skull_start_memcheck() {
    valgrind --tool=memcheck --leak-check=full \
        skull-engine -c $skull_config
}

skull_start_gdb() {
    echo "After gdb started, type 'run -c $skull_config'"
    echo ""
    gdb skull-engine
}

skull_start_strace() {
    strace -f skull-engine -c $skull_config
}
################################## End of Utils ################################

if [ $# = 0 ]; then
    echo "Error: Missing parameters" >&2
    usage >&2
    exit 1
fi

# 1. Parse the parameters
skull_config=""
memcheck=false
run_by_gdb=false
run_by_strace=false

args=`getopt -a \
        -o c:h \
        -l memcheck,gdb,strace,help \
        -n "skull-start.sh" -- "$@"`
if [ $? != 0 ]; then
    echo "Error: Invalid parameters" >&2
    usage >&2
    exit 1
fi

eval set -- "$args"

while true; do
    case "$1" in
        -c)
            shift
            skull_config=$1
            shift
            ;;
        --memcheck)
            shift
            memcheck=true
            ;;
        --gdb)
            shift
            run_by_gdb=true
            ;;
        --strace)
            shift
            run_by_strace=true
            ;;
        -h|--help)
            shift
            usage >&2
            exit 0
            ;;
        --)
            shift; break
            ;;
        (*)
            echo "Error: Unknown parameter $1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

# 2. Check parameters
if [ -z "$skull_config" ]; then
    echo "Fatal: Missing skull parameters" >&2
    usage >&2
    exit 1
fi

# 3. Rrepare the environment variables
skull_rundir=`dirname $skull_config`
export LD_LIBRARY_PATH=$skull_rundir/lib

# 4. Put some custom settings/actions here...

# 5. Start skull
if $memcheck; then
    skull_start_memcheck
elif $run_by_gdb; then
    skull_start_gdb
elif $run_by_strace; then
    skull_start_strace
else
    skull_start
fi
