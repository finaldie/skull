#!/usr/bin/env python3

import os
import sys
import time
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

IDX_TIME       = LOG_PREFIX + 0
IDX_TAG        = LOG_PREFIX + 2
IDX_PROTO      = LOG_PREFIX + 4
IDX_DATA_ADDR  = LOG_PREFIX + 5
IDX_DATA_SIZE  = LOG_PREFIX + 6
IDX_RET_ADDR   = LOG_PREFIX + 11
IDX_FRAME_START = LOG_PREFIX + 12

ADDR2LINE_CMD  = "addr2line -e {} -fpC {}"

MEMLEAK_THRESHOLD = 10 # N times larger than avg latency

MAX_FRAME         = 9  # Besides first caller, how many additional frames
MAX_REPORT        = 10
NUM_OF_PROCESSED  = 100

MEMLEAK_WHITELIST = ['deps/flibs/fhash']

# =========== Global non-static variables ===========
DEBUG   = False
EXTRACT = False

TRACING_START_TIME = 0
TRACING_END_TIME   = 0

REPORT_START_TIME  = 0
REPORT_END_TIME    = 0

# Memory usage map during tracing
"""
MemMap is for tracking the runtime alloc/dalloc actions, then we can use it
to calculate the current usage. Also if there is memleak, we can get the hints
from it.

MemMap = {
    skull.core: {
        usage: 10240,
        nalloc: 10,
        ndalloc: 10,
        details: {
            '0xFF45aac': {
                node: 'skull-engine',
                sz: 1024,
                saddr: 0x7524fabcd,
                naddr: 0x2524f,
                frames: [saddr1, saddr2, ...],
                time: 99123.123456789,
                start: 99123.123456789,
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
    '0xfdabb01_0aafdf_100': {
        'count' : 10,
        'alloced_in' : 'skull.core',
        'alloced': {
            node: 'skull-engine',
            saddr: 0x7ffafdc,
            naddr: 0x7faad,
            frames: [saddr1, saddr2, ...]
            sz: 1024,
        },
        'freed_in': 'module.response',
        'freed': {
            node: 'skull-engine',
            saddr: 0x8ffafdc,
            naddr: 0x4dacd,
            frames: [saddr1, saddr2, ...]
            sz: 1024,
        },
    }
    ...
}
"""
CrossScope = {}

"""
MemStat dict is to maintain the file -> usage mappings, to give us a better
vision of mem usage.

MemStat = {
    sAddr: {
        skull.core: {
            usage: 1024,
            nalloc: 10,
            ndalloc: 10,
            node: skull-engine,
            naddr: 0x7faad,
            frames: [saddr1, saddr2, ...]
            cost: 12341241.123456789,
            maxCost: 1000.123456789,
        },
        ...
    },
    ...
}
"""
MemStat = {}

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

        range:str    = varList[0]
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

def recordAction(lineArray, sAddr, addrMeta, frames):
    proto      = lineArray[IDX_PROTO] # e.g. malloc, free..
    timeNs     = float(lineArray[IDX_TIME])
    tag        = lineArray[IDX_TAG]   # e.g. master:skull.core
    engineName = tag.split(':')[0]    # e.g. master, worker, bio
    scopeName  = tag.split(':')[1]    # e.g. skull.core, module.request, svc.tt
    dataAddr   = lineArray[IDX_DATA_ADDR]
    dataSz     = lineArray[IDX_DATA_SIZE]
    pathName   = addrMeta[0]
    nAddr      = addrMeta[3]          # The real addr after fixing offset

    block:dict = MemMap.setdefault(scopeName, {})
    block.setdefault('usage', 0)
    block.setdefault('nalloc', 0)
    block.setdefault('ndalloc', 0)
    details:dict = block.setdefault('details', {})

    if proto in PROTOCOL_ALLOC:
        createRecord(scopeName, block, timeNs, dataAddr, int(dataSz), pathName,
                sAddr, nAddr, frames)

    elif proto == PROTOCOL_REALLOC:
        # 1. Split old and new size and pointers
        oldSz  = int(dataSz.split('-')[0])
        newSz  = int(dataSz.split('-')[1])
        diffSz = newSz - oldSz

        oldPtr = dataAddr.split('-')[0]
        newPtr = dataAddr.split('-')[1]

        # 2. update counters and detail.local accordingly
        if oldPtr == '(nil)': # pure allocation
            createRecord(scopeName, block, timeNs, newPtr, newSz, pathName,
                    sAddr, nAddr, frames)
        elif newPtr == '(nil)': # pure free
            removeRecord(scopeName, block, timeNs, oldPtr, oldSz, pathName,
                    sAddr, nAddr, frames)
        else: # movement
            if oldPtr != newPtr: # data moved
                createRecord(scopeName, block, timeNs, newPtr, newSz, pathName,
                        sAddr, nAddr, frames)
                removeRecord(scopeName, block, timeNs, oldPtr, oldSz, pathName,
                        sAddr, nAddr, frames)
            else: # data not moved, buffer got expanded
                detail = details.get(oldPtr)

                if detail is not None:
                    detail['sz']   += newSz

                    # The block usage only be updated if the record exist
                    block['usage'] += diffSz

    else: # free action
        removeRecord(scopeName, block, timeNs, dataAddr, int(dataSz), pathName,
                sAddr, nAddr, frames)

def createRecordData(timeSec:float, dataSz:int, node, sAddr, nAddr, frames):
    return {
        'node'  : node,
        'sz'    : dataSz,
        'saddr' : sAddr,
        'naddr' : nAddr,
        'frames': frames,
        'time'  : timeSec,
        'start' : time.clock_gettime(time.CLOCK_MONOTONIC),
    }

def createRecord(scopeName, block:dict, timeNs, dataAddr, dataSz:int, pathName,
        sAddr, nAddr, frames):
    # 1. update counters
    block['usage']  += dataSz
    block['nalloc'] += 1

    # 2. update details.location
    details = block.get('details')
    details[dataAddr] = createRecordData(
            timeNs, dataSz, pathName, sAddr, nAddr, frames)

    # 3. update mem stat
    recordMemStat(scopeName, pathName, sAddr, nAddr, frames, True, dataSz)

def removeRecord(scopeName, block:dict, timeNs, dataAddr, dataSz:int, pathName,
        sAddr, nAddr, frames):
    global DEBUG

    if dataAddr == "(nil)":
        return

    details = block.get('details')

    # 1. Find the record
    detail:dict = details.get(dataAddr)

    # 2. If found, update counters
    if detail is None:
        # ignore
        if DEBUG: print("Unmatched free: {}, try to free it cross scope".format(dataAddr))
        collectCrossScope(scopeName, timeNs, dataAddr, dataSz, pathName, sAddr,
                nAddr, frames)
        return

    # 3. Record stat. Notes: need to use the address information from allocation
    # block
    recordMemStat(scopeName, pathName, detail['saddr'], detail['naddr'],
            detail['frames'], False, dataSz, timeNs - detail['time'])

    # 4. Update counters and remove the record from dict
    block['usage']   -= int(dataSz)
    block['ndalloc'] += 1

    details.pop(dataAddr)

def collectCrossScope(rawScopeName, timeNs, dataAddr, dataSz, pathName, sAddr,
        nAddr, frames):
    for name in MemMap:
        block = MemMap[name]

        if dataAddr in block['details']:
            record = block['details'][dataAddr]
            if record['sz'] != dataSz:
                print(" - Warning: Unmatched size for cross-scope data free: " \
                      "alloced: {}, freed: {}".format(record['sz'], dataSz))

            allocAddr = record['saddr']

            key = "{}_{}_{}".format(allocAddr, sAddr, dataSz)
            crossScopeRecord = CrossScope.get(key)

            if crossScopeRecord is None:
                CrossScope[key] = {
                    'alloced'   : record,
                    'alloced_in': name,
                    'freed'     : createRecordData(timeNs, dataSz, pathName,
                                                   sAddr, nAddr, frames),
                    'freed_in'  : rawScopeName,
                    'count'     : 1
                }
            else:
                crossScopeRecord['count'] += 1

            if DEBUG: print(" - Cross scope data freed: {}".format(dataAddr))
            removeRecord(name, block, timeNs, dataAddr, dataSz, pathName, sAddr,
                    nAddr, frames)
            break

def _createMemStat(node, nAddr, frames):
    return {
        'usage'  : 0,
        'nalloc' : 0,
        'ndalloc': 0,
        'node'   : node,
        'naddr'  : nAddr,
        'frames' : frames,
        'cost'   : 0.0,
        'maxCost': -1.0,
    }

def recordMemStat(scopeName, node, sAddr, nAddr, frames, isAlloc:bool, dataSz, cost=None):
    entry = MemStat.get(sAddr)
    if entry is None:
        if isAlloc is False: return

        MemStat[sAddr] = {}
        entry = MemStat[sAddr]

    block = entry.get(scopeName)
    if block is None:
        if isAlloc is False: return

        entry[scopeName] = _createMemStat(node, nAddr, frames)
        block = entry[scopeName]

    if isAlloc:
        block['usage']  += dataSz
        block['nalloc'] += 1
    else:
        block['usage']   -= dataSz
        block['ndalloc'] += 1
        block['cost']    += cost
        block['maxCost']  = max(cost, block['maxCost'])

def _getMemStat(sAddr, scopeName):
    block = MemStat.get(sAddr)
    if block is None: return None

    return block.get(scopeName)

def _calAvgNs(memStat:dict):
    return -1 if memStat['ndalloc'] == 0 else memStat['cost'] / memStat['ndalloc'] * 1000000000

def _reportMemStat():
    rawList = []

    totalUsage      = 0
    totalAllocCnt   = 0
    totalDeallocCnt = 0

    for sAddr in MemStat:
        for scope in MemStat[sAddr]:
            block = MemStat[sAddr][scope]

            avgNs = _calAvgNs(block)
            avgMs = 'n/a' if avgNs < 0 else "%.2f" % (avgNs / 1000000)

            maxCost = block['maxCost']
            maxMs = 'n/a' if maxCost < 0 else "%.2f" % (maxCost * 1000)

            record = {
                'usage'   : block['usage'],
                'scope'   : scope,
                'avgMs'   : avgMs,
                'maxMs'   : maxMs,
                'nalloc'  : block['nalloc'],
                'ndalloc' : block['ndalloc'],
                'node'    : os.path.basename(block['node']),
                'from'    : addr2line(block['node'], block['naddr'])
            }

            rawList.append(record)

            totalUsage      += block['usage']
            totalAllocCnt   += block['nalloc']
            totalDeallocCnt += block['ndalloc']

    sortedRes = sorted(rawList, key = lambda record: record['usage'],
            reverse = True)

    title = ['usage', 'scope', 'nalloc', 'ndalloc', 'avgMs', 'maxMs', 'node',
             'from']
    fmt   = '{:<10}{:<16}{:<8}{:<8}{:<10}{:<10}{:<25}{}'

    print("\n{:=^80}".format(' Memory Usage Report '))
    print(fmt.format(*title))

    for record in sortedRes:
        print(fmt.format(
            record['usage'],   record['scope'], record['nalloc'],
            record['ndalloc'], record['avgMs'], record['maxMs'],
            record['node'],    record['from']), end = '')

    print("\nTotal Usage: {} Bytes, #Alloc: {}, #Dealloc: {}".format(
          totalUsage, totalAllocCnt, totalDeallocCnt))

def _dumpFrames(frames, fmt):
    sz = len(frames)
    order = reversed(range(0, sz))

    for i in order:
        sAddri = frames[i]
        if sAddri == '(nil)':
            continue

        res:tuple = _findAddr(addrMaps, sAddri)

        node   = res[0] # e.g. /lib/x86_64-linux-gnu/ld-2.27.so
        nAddri = res[3] # Re-calculated address

        hrAddri:str = addr2line(node, nAddri)
        print(fmt.format(i + 1, sAddri, hrAddri), end = '')

def __dumpCrossScopeStacks(lineno, block, tag, scopeName):
    print()
    print("[{}] {} ({})".format(lineno, tag, scopeName))

    title = ['#F', 'Address', 'FrameInfo']
    fmt   = '{:<3}{:<20}{}'

    _dumpFrames(block['frames'], fmt)

    hrAddr1:str = addr2line(block['node'], block['naddr'])
    print(fmt.format(0, block['saddr'], hrAddr1), end = '')

def _dumpCrossScopeStacks(stacks):
    for record in stacks:
        lineno = record[0]
        block  = record[1]

        __dumpCrossScopeStacks(lineno, block['alloced'],
                'Allocation Stacks', block['alloced_in'])

        __dumpCrossScopeStacks(lineno, block['freed'],
                'Deallocation Stacks', block['alloced_in'])

def _reportCrossScope():
    rawList = []

    for key in CrossScope:
        detail = CrossScope[key]

        record = {
            'count'       : detail['count'],
            'size'        : detail['alloced']['sz'],
            'usage'       : detail['alloced']['sz'] * detail['count'],
            'alloced'     : detail['alloced'],
            'alloced_in'  : detail['alloced_in'],
            'freed'       : detail['freed'],
            'freed_in'    : detail['freed_in'],
        }

        rawList.append(record)

    sortedList = sorted(rawList, key = lambda record : record['usage'],
            reverse = True)

    print("\n{:=^80}".format(' Top {} CrossScope Malloc/Free Report '.format(
        MAX_REPORT)))

    if len(sortedList) == 0:
        print("Great work! No cross-scope issues found")
        return

    title = ['#', 'usage', 'size/c', 'times', 'alloced_in', 'freed_in']
    fmt   = "{:<3}{:<10}{:<10}{:<10}{:<16}{:<16}"

    print(fmt.format(*title))

    lineno   = -1
    hidenCnt = 0
    stacks = []

    for record in sortedList:
        lineno += 1

        if lineno >= MAX_REPORT:
            hidenCnt += 1
            continue

        print(fmt.format(
            lineno, record['usage'], record['size'], record['count'],
            record['alloced_in'], record['freed_in']))

        stacks.append((lineno, record))

    print("\nTotal {} records are hided\n".format(hidenCnt))
    _dumpCrossScopeStacks(stacks)

def _dumpMemLeakStacks(stacks):
    for record in stacks:
        lineno  = record[0]
        block   = record[1]
        hrAddr1 = record[2]

        sAddr1  = block['saddr']
        frames  = block['frames']

        print()
        print("[{}]".format(lineno))

        title = ['#F', 'Address', 'FrameInfo']
        fmt   = '{:<3}{:<20}{}'
        print(fmt.format(*title))

        _dumpFrames(frames, fmt)
        print(fmt.format(0, sAddr1, hrAddr1), end = '')

def _isMemLeakWhiteList(hrAddr):
    for item in MEMLEAK_WHITELIST:
        if item in hrAddr:
            return True

    return False


def _reportMemLeak():
    now = TRACING_END_TIME
    rawList  = []
    hidenCnt = 0

    for scopeName in MemMap:
        scope = MemMap[scopeName]
        details = scope['details']

        for dataAddr in details:
            block = details[dataAddr]
            sAddr = block['saddr']
            timeSec = block['start'] # use this, not 'time' field

            memStat = _getMemStat(sAddr, scopeName)
            avgNs   = _calAvgNs(memStat)
            maxNs   = memStat['maxCost'] * 1000000000
            latencyNs = (now - timeSec) * 1000000000

            if maxNs < 0:
                rawList.append(("Possible", scopeName, block, memStat,
                    latencyNs, avgNs, maxNs))
            else:
                if latencyNs > MEMLEAK_THRESHOLD * maxNs:
                    rawList.append(("Likely", scopeName, block, memStat,
                        latencyNs, avgNs, maxNs))
                else:
                    rawList.append(("Noleak", scopeName, block, memStat,
                        latencyNs, avgNs, maxNs))

    sortedList = sorted(rawList, key = lambda x: x[0])

    global DEBUG
    if DEBUG: pprint.pprint(MemMap)

    title = ['#', 'reason', 'ttlMs', 'avgMs', 'maxMs', 'usage', 'nalloc',
            'ndalloc', 'scope', 'node', 'from']
    fmt   = '{:<3}{:<10}{:<10}{:<10}{:<10}{:<10}{:<8}{:<8}{:<16}{:<25}{}'

    print("\n{:=^80}".format(' Top {} MemLeak Report (Experimental) '.format(
        MAX_REPORT)))
    print(fmt.format(*title))

    lineno = -1
    stacks = []

    for record in sortedList:
        reason    = record[0]
        scopeName = record[1]
        block     = record[2]
        memStat   = record[3]
        latencyNs = record[4]
        avgNs     = record[5]
        maxNs     = record[6]


        if lineno >= MAX_REPORT or reason == 'Noleak':
            hidenCnt += 1
            continue

        allocated_from = addr2line(block['node'], block['naddr'])
        if _isMemLeakWhiteList(allocated_from):
            hidenCnt += 1
            continue

        lineno += 1
        latencyMs = "%.2f" % (latencyNs / 1000000)
        avgMs = 'n/a' if avgNs < 0 else "%.2f" % (avgNs / 1000000)
        maxMs = 'n/a' if maxNs < 0 else "%.2f" % (maxNs / 1000000)

        print(fmt.format(
            lineno, reason, latencyMs, avgMs, maxMs, block['sz'],
            memStat['nalloc'], memStat['ndalloc'], scopeName,
            os.path.basename(block['node']), allocated_from), end = '')

        stacks.append((lineno, block, allocated_from))

    print("\nTotal {} data blocks are hided\n".format(hidenCnt))

    _dumpMemLeakStacks(stacks)

def report():
    global REPORT_START_TIME
    global REPORT_END_TIME

    REPORT_START_TIME = time.clock_gettime(time.CLOCK_MONOTONIC)

    _reportMemStat()
    _reportCrossScope()
    _reportMemLeak()

    REPORT_END_TIME = time.clock_gettime(time.CLOCK_MONOTONIC)

    print("{:=^80}".format(''))
    print("Tracing time: %.3f s, Report generation time: %.3f s" % (
        TRACING_END_TIME - TRACING_START_TIME,
        REPORT_END_TIME - REPORT_START_TIME
    ))
    print()

def addr2line(pathName, nAddr):
    global DEBUG

    output = subprocess.check_output(
                ADDR2LINE_CMD.format(pathName, nAddr), shell=True)

    hrAddr:str = output.decode('UTF-8')

    return hrAddr


def printTracing(lineArray, lineno, node, nAddr):
    global EXTRACT

    if EXTRACT:
        hrAddr = addr2line(node, nAddr)

        lineArray[IDX_RET_ADDR] = (
                hrAddr if not hrAddr.startswith('??') else "{}\n".format(node))

        print(" ".join(lineArray), end = '')

    elif lineno % NUM_OF_PROCESSED == 0:
        print("#", end = '')
        sys.stdout.flush()

def doAddrRemapping(addrMaps:dict, executable:str, inputFile:str):
    global DEBUG
    global TRACING_START_TIME
    global TRACING_END_TIME

    TRACING_START_TIME = time.clock_gettime(time.CLOCK_MONOTONIC)

    handle = sys.stdin
    if inputFile is not None:
        try:
            handle = open(inputFile, 'r')
        except Exception as e:
            print("Error: Cannot open {}: {}".format(inputFile, str(e)))
            raise

    lineno = 0

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
        if len(array) < ITEMS_PER_LINE:
            if DEBUG: print("skip line, no enough items per line: {}".format(line));
            continue

        lineno += 1
        proto = array[IDX_PROTO]    # proto like malloc/calloc
        sAddr = array[IDX_RET_ADDR] # function return address

        frames = []
        for i in range(IDX_FRAME_START, IDX_FRAME_START + MAX_FRAME):
            frames.append(array[i])

        res:tuple = _findAddr(addrMaps, sAddr)

        if res[0] is None:
            print("Warning: cannot find address in maps: {}".format(sAddr))
            print("{}".format(line), end = '')
        else:
            if DEBUG: print("Found addr {} in {}, base: {}, range: {} -> {}".format(
                sAddr, res[0], res[1], res[2], res[3]))

            pathName = res[0] # e.g. /lib/x86_64-linux-gnu/ld-2.27.so
            newAddr  = res[3] # Re-calculated address

            try:
                printTracing(array, lineno, pathName, newAddr)

                # Collect all usage data into global dict
                recordAction(array, sAddr, res, frames)

            except KeyboardInterrupt:
                if DEBUG: print("KeyboardInterrupt, exit...")
                break

    TRACING_END_TIME = time.clock_gettime(time.CLOCK_MONOTONIC)

    if inputFile is not None:
        handle.close()

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
    print ("  skull-addr-remapping.py -e exe -p pid [-f inputDataFile] [-a addr] [-d] [-D]")

if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()
        sys.exit(1)

    try:
        pid   = 0
        input = None
        addr  = None
        executable = ""
        opts, args = getopt.getopt(sys.argv[1:7], 'e:p:f:a:dD')

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
                EXTRACT = True
            elif op == "-D":
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

            hrAddr:str = addr2line(pathName, newAddr)
            print("Addr2line(utf-8): {}".format(hrAddr))
        else:
            doAddrRemapping(addrMaps, executable, input)
            report()

    except KeyboardInterrupt as e:
        if DEBUG: print("KeyboardInterrupt: {}".format(e))
        report()
    except Exception as e:
        print("Fatal: " + str(e))
        usage()
        raise

