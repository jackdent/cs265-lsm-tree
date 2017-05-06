import os
import subprocess
import sys
from tempfile import TemporaryFile

ROOT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)
TEST_ROOT_DIR = os.path.join(ROOT_DIR, "test")
LSM_TREE_EXECUTABLE = os.path.join(ROOT_DIR, "bin", "lsm")
TEST_DIR_PREFIX = "test-"
INFILE = "in"
OUTFILE = "out"
PARAMFILE = "params"
SEPARATOR = "-" * 80

def run_test(test_dir):
    cwd = os.getcwd()
    os.chdir(test_dir)

    with open(INFILE, 'r') as infile, open(OUTFILE, 'r') as outfile, TemporaryFile('r') as dump:
        try:
            params = open(PARAMFILE, 'r').read().rstrip().split(' ')
        except:
            params = []

        subprocess.call([LSM_TREE_EXECUTABLE] + params, stdin=infile, stdout=dump)
        dump.seek(0)

        expected, obtained = outfile.read(), dump.read()

        if expected == obtained:
            print("* Test {} SUCCEEDED".format(test_dir))
        else:
            print("x Test {} FAILED".format(test_dir))
            print(SEPARATOR)

            print("Expected:")
            print(SEPARATOR)
            print(expected)
            print(SEPARATOR)

            print("Obtained:")
            print(SEPARATOR)
            print(obtained)
            print(SEPARATOR)

    os.chdir(cwd)

if __name__ == "__main__":
    if len(sys.argv) == 1:
        for entry in os.listdir(TEST_ROOT_DIR):
            if entry.startswith(TEST_DIR_PREFIX):
                run_test(os.path.join(TEST_ROOT_DIR, entry))
    elif len(sys.argv) == 2:
        run_test(os.path.join(TEST_ROOT_DIR, TEST_DIR_PREFIX + sys.argv[1]))
    else:
        print("Usage: python lsm-test.py [i]. If no test is specified, all tests will be run.")
        sys.exit(1)
