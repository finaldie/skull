#!/usr/bin/env python3

import sys
import os
import getopt
import string
import pprint
import subprocess

known_regions = ["[heap]", "[stack]", "[vvar]", "[vdso]", "[vsyscall]"]

DEBUG = False

def loadAddrMaps(pid:int):
    """Load Address Mappings from /proc/pid/maps
    The mapping object format like this:
    maps -> {
        '/usr/bin/executable': [(start1, end1), (start2, end2) ...],
        '/lib/x86_64-linux-gnu/libc-2.27.so': [(start1, end1), (start2, end2) ...],
        ...
    }
    """

    maps = {}
    procMaps = None
    mapFileName = "/proc/{}/maps".format(pid)
    if DEBUG: print("map file: {}".format(mapFileName))

    try:
        procMaps = open(mapFileName, 'r')
    except Exception as e:
        print("Error: Cannot open {}: {}".format(mapFileName, str(e)))
        raise

    while True:
        line:str = procMaps.readline()
        if DEBUG: print("line: {}".format(line))
        if not line:
            break

        varList = line.strip('\n').split()
        if DEBUG: pprint.pprint(varList)
        if len(varList) < 6:
            continue

        range:str = varList[0]
        pathName:str = varList[5]
        if DEBUG: print("range: {} ,pathName: {}".format(range, pathName))

        if pathName not in known_regions:
            entry:list = maps.get(pathName) or []
            maps[pathName] = entry

            rstart = range.split('-')[0]
            rend   = range.split('-')[1]

            entry.append((rstart,rend))

    procMaps.close()

    if DEBUG: print("dump maps:")
    if DEBUG: pprint.pprint(maps) # debug
    return maps


protocols = ["malloc", "calloc", "realloc", "free", "posix_memalign", "aligned_alloc"]
MEM_TAG        = "[MEM_TRACE]"
LOG_PREFIX     = 6
ITEMS_PER_LINE = LOG_PREFIX + 12

PROTO_IDX      = LOG_PREFIX + 4
RET_ADDR_IDX   = LOG_PREFIX + 11

ADDR2LINE_CMD  = "addr2line -e {} -fpC {}"

def doAddrRemapping(addrMaps:dict, executable:str, inputFile:str):
    handle = sys.stdin
    if inputFile is not None:
        try:
            handle = open(inputFile, 'r')
        except Exception as e:
            print("Error: Cannot open {}: {}".format(inputFile, str(e)))
            raise

    while True:
        try:
            line:str = handle.readline()
        except KeyboardInterrupt:
            if DEBUG: print("KeyboardInterrupt, exit...")
            break
        except Exception as e:
            print("Exception happend during read from stdin: {}".format(e))
            break

        if DEBUG: print("original line: {}".format(line), end = '')
        if not line:
            break

        if line.find(MEM_TAG) == -1:
            if DEBUG: print("skip line, no tag {}: {}".format(MEM_TAG, line))
            continue

        array = line.split()
        if len(array) != ITEMS_PER_LINE:
            if DEBUG: print("skip line, no enough items per line: {}".format(line));
            continue

        proto = array[PROTO_IDX]  # proto like malloc/calloc
        addr  = array[RET_ADDR_IDX] # function return address

        res:tuple = _findAddr(addrMaps, addr)
        if res[0] is None:
            print("Warning: cannot find address in maps: {}".format(addr))
            print("{}".format(line), end = ' ')
        else:
            if DEBUG: print("Found addr {} in {}, base: {}, range: {} -> {}".format(
                addr, res[0], res[1], res[2], res[3]))

            newAddr = res[3]

            try:
                output = subprocess.check_output(
                        ADDR2LINE_CMD.format(executable, newAddr), shell=True)
                humanReadableAddr = output.decode('UTF-8')

                array[RET_ADDR_IDX] = humanReadableAddr
                print(" ".join(array), end = ' ')

            except KeyboardInterrupt:
                if DEBUG: print("KeyboardInterrupt, exit...")
                break

    if inputFile is not None:
        handle.close()
    return

def _findAddr(addrMaps:dict, saddr:str):
    iaddr = int(saddr, 16)

    for pathName in addrMaps:
        ranges:list = addrMaps.get(pathName)
        baseline = ranges[0][0]
        hBaseline = int(baseline, 16)

        for range in ranges:
            rstart = int(range[0], 16)
            rend   = int(range[1], 16)

            if iaddr >= rstart and iaddr <= rend:
                # Found it
                return (pathName, baseline, range, hex(iaddr - hBaseline))

    return (None, None, None, None)

################################################################################
def usage():
    print ("usage:")
    print ("  skull-addr-remapping.py -e exe -p pid [-f inputDataFile] [-a addr] [-d]")

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        pid = 0
        input = None
        addr = None
        executable = ""
        opts, args = getopt.getopt(sys.argv[1:7], 'e:p:f:a:d')

        for op, value in opts:
            if op == "-e":
                executable = value
            elif op == "-p":
                pid = int(value)
            elif op == "-f":
                input = value
            elif op == "-a":
                addr = value
            elif op == "-d":
                DEBUG = True

        # Now run the process func according the mode
        if pid <= 0:
            print ("Fatal: Invalid pid: %d" % pid)
            sys.exit(1)

        addrMaps = loadAddrMaps(pid)
        doAddrRemapping(addrMaps, executable, input)

        if addr is not None:
            res = _findAddr(addrMaps, addr)
            pprint.pprint(res)

    except KeyboardInterrupt as e:
        if DEBUG: print("KeyboardInterrupt: {}".format(e))
    except Exception as e:
        print("Fatal: " + str(e))
        usage()
        raise

