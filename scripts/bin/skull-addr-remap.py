#!/usr/bin/env python3

import sys
import os
import getopt
import string
import pprint
import subprocess

# =========== Global static variables ===========
KNOWN_REGIONS  = ["[heap]", "[stack]", "[vvar]", "[vdso]", "[vsyscall]"]

PROTOCOL_ALLOC = ["malloc", "calloc", "posix_memalign", "aligned_alloc"]
PROTOCOL_REALLOC = "realloc"
PROTOCOL_FREE    = "free"

MEM_TAG        = "[MEM_TRACE]"
LOG_PREFIX     = 6
ITEMS_PER_LINE = LOG_PREFIX + 12

IDX_TAG        = LOG_PREFIX + 2
IDX_PROTO      = LOG_PREFIX + 4
IDX_DATA_ADDR  = LOG_PREFIX + 5
IDX_DATA_SIZE  = LOG_PREFIX + 6
IDX_RET_ADDR   = LOG_PREFIX + 11

ADDR2LINE_CMD  = "addr2line -e {} -fpC {}"

# =========== Global non-static variables ===========
DEBUG = False

# Memory usage map during tracing
"""
MemMap = {
    skull.core: {
        usage: 10240,
        nalloc: 10,
        ndalloc: 10,
        details: {
            '0xFF45aac': {
                node: 'skull-engine',
                from: 'sk_init at main.c:10',
                sz: 1024,
                saddr: 0x2524f
                naddr: 0x2524f
            },
            ...
        }
    },

    module.request: {
        same as above
        ...
    },

    service.test: {
        same as above
        ...
    }
}
"""
MemMap = {}

"""
CrossScope map for recording how many data are transfer from one scope to
 another.

CrossScope = {
    '0xfdabb01': {
        'alloced': {
            node: 'skull-engine',
            from: 'sk_init at main.c:10',
            sz: 1024,
        },
        'freed': {
            node: 'skull-engine',
            from: 'sk_release at main.c:20',
            sz: 1024,
        },
    }
    ...
}
"""
CrossScope = {}

def loadAddrMaps(pid:int):
    """Load Address Mappings from /proc/pid/maps
    The mapping object format like this:
    maps -> {
        '/usr/bin/executable': [(start1, end1), (start2, end2) ...],
        '/lib/x86_64-linux-gnu/libc-2.27.so': [(start1, end1), (start2, end2) ...],
        ...
    }
    """

    global DEBUG

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

        if pathName not in KNOWN_REGIONS:
            entry:list = maps.get(pathName) or []
            maps[pathName] = entry

            rstart = range.split('-')[0]
            rend   = range.split('-')[1]

            entry.append((rstart,rend))

    procMaps.close()

    if DEBUG: print("dump maps:")
    if DEBUG: pprint.pprint(maps) # debug
    return maps

def recordAction(lineArray, addrMeta, sAddr, hrAddr):
    proto = lineArray[IDX_PROTO]   # e.g. malloc, free..
    tag   = lineArray[IDX_TAG]     # e.g. master:skull.core
    engineName = tag.split(':')[0] # e.g. master, worker, bio
    scopeName  = tag.split(':')[1] # e.g. skull.core, module.request, service.tt
    dataAddr   = lineArray[IDX_DATA_ADDR]
    dataSz     = lineArray[IDX_DATA_SIZE]
    pathName   = addrMeta[0]
    nAddr      = addrMeta[3]       # The real addr after fixing offset

    block:dict = MemMap.setdefault(scopeName, {})
    block.setdefault('usage', 0)
    block.setdefault('nalloc', 0)
    block.setdefault('ndalloc', 0)
    details:dict = block.setdefault('details', {})

    if proto in PROTOCOL_ALLOC:
        createRecord(block, dataAddr, int(dataSz), pathName, hrAddr, sAddr, nAddr)

    elif proto == PROTOCOL_REALLOC:
        # 1. Split old and new size and pointers
        oldSz  = int(dataSz.split('-')[0])
        newSz  = int(dataSz.split('-')[1])
        diffSz = newSz - oldSz

        oldPtr = dataAddr.split('-')[0]
        newPtr = dataAddr.split('-')[1]

        # 2. update counters and detail.local accordingly
        if oldPtr == '(nil)': # pure allocation
            createRecord(block, newPtr, newSz, pathName, hrAddr, sAddr, nAddr)
        elif newPtr == '(nil)': # pure free
            removeRecord(scopeName, block, oldPtr, oldSz, pathName, hrAddr, sAddr, nAddr)
        else: # movement
            if oldPtr != newPtr: # data moved
                createRecord(block, newPtr, newSz, pathName, hrAddr, sAddr, nAddr)
                removeRecord(scopeName, block, oldPtr, oldSz, pathName, hrAddr, sAddr, nAddr)
            else: # data not moved, buffer got expanded
                detail = details.get(oldPtr)

                if detail is not None:
                    detail['sz']   += newSz

                    # The block usage only be updated if the record exist
                    block['usage'] += diffSz

    else: # free action
        removeRecord(scopeName, block, dataAddr, dataSz, pathName, hrAddr, sAddr, nAddr)

def createRecordData(dataSz:int, pathName, hrAddr, sAddr, nAddr):
    return {
        'node': pathName,
        'from': hrAddr,
        'sz'  : dataSz,
        'saddr': sAddr,
        'naddr': nAddr
    }

def createRecord(block:dict, dataAddr, dataSz:int, pathName, hrAddr, sAddr, nAddr):
    # 1. update counters
    block['usage']  += dataSz
    block['nalloc'] += 1

    # 2. update details.location
    details = block.get('details')
    details[dataAddr] = createRecordData(dataSz, pathName, hrAddr, sAddr, nAddr)

def removeRecord(scopeName, block:dict, dataAddr, dataSz:int, pathName, hrAddr, sAddr, nAddr):
    if dataAddr == "(nil)":
        return

    details = block.get('details')

    # 1. Find the record
    detail:dict = details.get(dataAddr)

    # 2. If found, update counters
    if detail is None:
        # ignore
        print("Unmatched free: {}, try to free it cross scope".format(dataAddr))
        collectCrossScope(scopeName, dataAddr, dataSz, pathName, hrAddr, sAddr, nAddr)
        return

    block['usage']   -= int(dataSz)
    block['ndalloc'] += 1

    details.pop(dataAddr)

def collectCrossScope(scopeName, dataAddr, dataSz, pathName, hrAddr, sAddr, nAddr):
    for name in MemMap:
        block = MemMap[name]

        if dataAddr in block['details']:
            record = block['details'][dataAddr]
            allocAddr = record['naddr']

            key = "{}_{}_{}".format(allocAddr, nAddr, dataSz)
            crossScopeRecord = CrossScope.get(key)

            if crossScopeRecord is None:
                CrossScope[key] = {
                    'alloced': record,
                    'alloced_in': name,
                    'freed': createRecordData(dataSz, pathName, hrAddr, sAddr, nAddr),
                    'freed_in': scopeName,
                    'count': 1
                }
            else:
                crossScopeRecord['count'] += 1

            print("cross scope free: {}".format(dataAddr))
            removeRecord(scopeName, block, dataAddr, dataSz, pathName, hrAddr, sAddr, nAddr)

def report():
    print("Report:")
    pprint.pprint(MemMap)

    print("CrossScope:")
    pprint.pprint(CrossScope)
    return

def doAddrRemapping(addrMaps:dict, executable:str, inputFile:str):
    global DEBUG

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

        proto = array[IDX_PROTO]    # proto like malloc/calloc
        addr  = array[IDX_RET_ADDR] # function return address

        res:tuple = _findAddr(addrMaps, addr)
        if res[0] is None:
            print("Warning: cannot find address in maps: {}".format(addr))
            print("{}".format(line), end = ' ')
        else:
            if DEBUG: print("Found addr {} in {}, base: {}, range: {} -> {}".format(
                addr, res[0], res[1], res[2], res[3]))

            pathName = res[0] # e.g. /lib/x86_64-linux-gnu/ld-2.27.so
            newAddr  = res[3] # Re-calculated address

            try:
                output = subprocess.check_output(
                        ADDR2LINE_CMD.format(pathName, newAddr), shell=True)

                if DEBUG: print("raw addr2line output: {}".format(output))
                humanReadableAddr:str = output.decode('UTF-8')

                array[IDX_RET_ADDR] = humanReadableAddr.startswith('??') and humanReadableAddr or "{}\n".format(pathName)
                print(" ".join(array), end = ' ')

                # Collect all usage data into global dict
                recordAction(array, res, addr, humanReadableAddr)

            except KeyboardInterrupt:
                if DEBUG: print("KeyboardInterrupt, exit...")
                break

    if inputFile is not None:
        handle.close()

    report()
    return

def _findAddr(addrMaps:dict, saddr:str):
    """
        Find the absolute address for the source address

        Return a tuple contains: pathName, rangeBegin, rangeEnd, absoluteAddr

        TODO: Use Segment/Interval Tree to optimize it. The time complexity will
              be reduced from O(n) to O(logn)
    """
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

        if addr is not None:
            res = _findAddr(addrMaps, addr)
            pprint.pprint(res)
            pathName = res[0] # e.g. /lib/x86_64-linux-gnu/ld-2.27.so
            newAddr  = res[3]

            cmd = ADDR2LINE_CMD.format(pathName, newAddr)
            print("addr2line cmd: {}".format(cmd))
            output = subprocess.check_output(cmd, shell=True)

            print("raw addr2line output: {}".format(output))
            hrAddr:str = output.decode('UTF-8')
            print("addrline output after utf-8 decode: {}".format(hrAddr))
        else:
            doAddrRemapping(addrMaps, executable, input)

    except KeyboardInterrupt as e:
        if DEBUG: print("KeyboardInterrupt: {}".format(e))
        report()
    except Exception as e:
        print("Fatal: " + str(e))
        usage()
        raise

