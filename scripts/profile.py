import os
import subprocess
import sys
import time

ROOT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)
LSM_TREE_EXECUTABLE = os.path.join(ROOT_DIR, "bin", "lsm")

def profile(args_file):
    with open(os.path.join(os.getcwd(), args_file), "r") as args_list:
        for args in args_list:
            # Strip new line character
            args = args[:-1].split(' ')

            # Time the workload
            start = time.time()
            subprocess.call([LSM_TREE_EXECUTABLE] + args, stdout=open(os.devnull, "w"))
            end = time.time()
            print(end - start)

            # Reset STDIN so the next call reads the same workload
            sys.stdin.seek(0)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print('Usage: python profile.py arguments.txt <workload.txt')
        sys.exit(1)
    else:
        profile(sys.argv[1])
