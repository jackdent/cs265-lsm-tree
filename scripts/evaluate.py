# ================================================
# = Harvard University | CS265 | Systems Project =
# ================================================
# ========== LSM TREE WORKLOAD EVALUATOR =========
# ================================================
# Contact:
# ========
# - Kostas Zoumpatianos <kostas@seas.harvard.edu>
# - Michael Kester <kester@eecs.harvard.edu>

import sys
import time
import struct
from blist import sorteddict, sortedlist

# ------------------------------------------------
#                     Stats
# ------------------------------------------------
puts = 0
successful_gets = 0
failed_gets = 0
ranges = 0
successful_deletes = 0
failed_deletes = 0
loads = 0
time_elapsed = 0

# Logging events
def log(action, verbose):
    global puts
    global successful_gets
    global failed_gets
    global ranges
    global successful_deletes
    global failed_deletes
    global loads
    if action == "PUT":
        puts += 1
    elif action == "SUCCESFUL_GET":
        successful_gets += 1
    elif action == "FAILED_GET":
        failed_gets += 1
    elif action == "RANGE":
        ranges += 1
    elif action == "SUCCESSFUL_DELETE":
        successful_deletes += 1
    elif action == "FAILED_DELETE":
        failed_deletes += 1
    elif action == "LOAD":
        loads += 1
    if verbose:
        print action

# Printing stats
def print_stats(time_elapsed):
    global puts
    global successful_gets
    global failed_gets
    global ranges
    global successful_deletes
    global failed_deletes
    global loads
    print "------------------------------------"
    print "PUTS", puts
    print "SUCCESFUL_GETS", successful_gets
    print "FAILED_GETS", failed_gets
    print "RANGES", ranges
    print "SUCCESSFUL_DELS", successful_deletes
    print "FAILED_DELS", failed_deletes
    print "LOADS", loads
    print "TIME_ELAPSED", time_elapsed
    print "------------------------------------"

# ------------------------------------------------
#                   Main function
# ------------------------------------------------
if __name__ == "__main__":
    # Options
    verbose = False
    show_output = False

    # Initialize sorted dictionary (key value store)
    db = sorteddict({})

    # Open file
    f = open(sys.argv[1])

    # Start reading workload
    start = time.time()
    for line in f:
        if line:
            # PUT
            if line[0] == "p":
                (key, val) = map(int, line.split(" ")[1:3])
                db[key] = val
                log("PUT", verbose)
            # GET
            if line[0] == "g":
                key = int(line.split(" ")[1])
                try:
                    val = db[key]
                    if show_output:
                        print val
                    log("SUCCESFUL_GET", verbose)
                except:
                    if show_output:
                        print ""
                    log("FAILED_GET", verbose)
            # RANGE
            if line[0] == "r":
                (range_start, range_end) = map(int, line.split(" ")[1:3])
                sorted_keys = sortedlist(db.keys())
                valid_keys = sorted_keys[range_start:range_end]
                valid_vals = map(lambda x: db[x], valid_keys)
                res = zip(valid_keys, valid_vals)
                if show_output:
                    print " ".join(map(lambda x: str(x[0])+":"+str(x[1]), res))
                log("RANGE", verbose)
            # DELETE
            if line[0] == "d":
                key = int(line.split(" ")[1])
                try:
                    del db[key]
                    log("SUCCESSFUL_DELETE", verbose)
                except:
                    log("FAILED_DELETE", verbose)
                    pass
            # LOAD
            if line[0] == "l":
                log("LOAD", verbose)
                filename = line[2:-1]
                with open(filename, "rb") as f:
                    while True:
                        buf = f.read(4)
                        if not buf: break
                        key = struct.unpack('i', buf)
                        buf = f.read(4)
                        if not buf: break
                        val = struct.unpack('i', buf)
                        db[key[0]] = val[0]
                        log("PUT", verbose)
    # Done
    end = time.time()
    time_elapsed = (end-start)

    # Print stats
    if not verbose:
        print_stats(time_elapsed)

    # Closing file
    f.close()
