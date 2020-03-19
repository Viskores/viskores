#!/usr/bin/env python3
"""
compare-benchmarks.py - VTKm + Google Benchmarks compare.py
"""

import getopt
import subprocess
import sys
import time
import os

CURRENT_DIR = os.path.dirname(os.path.realpath(__file__))
COMPARE_PY_PATH = os.path.join(CURRENT_DIR, 'compare.py')
COMPARE_PY = sys.executable + " " + COMPARE_PY_PATH

class Bench():
    def __init__(self):
        self.__cmd = None

    @property
    def cmd(self):
        return self.__cmd

    @cmd.setter
    def cmd(self, c):
        self.__cmd = c

    def launch(self):
        output_file = "bench-%d.json" % time.time()
        cmd_exec = "%s --benchmark_out=%s --benchmark_out_format=json" \
                % (self.cmd, output_file)
        print(cmd_exec)
        subprocess.call(cmd_exec, shell=True)
        return output_file

def print_help(error_msg = None):
    if error_msg != None:
        print(error_msg)

    print("usage: compare-benchmarks <opts>\n" \
            " --benchmark1='<benchmark1> [arg1] [arg2] ...'"\
            " [--filter1=<filter1>]\n"\
            " --benchmark2='<benchmark2> [arg1] [arg2] ...'"\
            " [--filter2=<filter2>]\n"\
            " -- [-opt] benchmarks|filters|benchmarksfiltered\n\n" \
            "compare.py help:")

    subprocess.call(COMPARE_PY, shell=True)
    sys.exit(0)

# -----------------------------------------------------------------------------
def main():
    is_filters = False
    filter1 = str()
    filter2 = str()
    bench1 = Bench()
    bench2 = Bench()

    options, remainder = getopt.gnu_getopt(sys.argv[1:], '',
            ['help','benchmark1=', 'benchmark2=', 'filter1=', 'filter2='])

    for opt, arg in options:
        if opt == "--benchmark1":
            bench1.cmd = arg

        if opt == "--benchmark2":
            bench2.cmd = arg

        if opt == "--filter1":
            filter1 = arg

        if opt == "--filter2":
            filter2 = arg

        if opt == "--help":
            print_help()

    if bench1.cmd == None:
        print_help("ERROR: no benchmarks chosen")

    for arg in remainder:
        if arg == "filters":
           is_filters = True

    if is_filters and bench2.cmd != None:
        print_help("ERROR: filters option can only accept --benchmark1= and --filter1")

    b1_output = bench1.launch()
    b2_output = bench2.launch() if not is_filters else filter1 + " " + filter2

    cmd = "%s %s %s %s" % (COMPARE_PY, " ".join(remainder), b1_output, b2_output)
    print(cmd)
    subprocess.call(cmd, shell=True)

    os.remove(b1_output)

    if not is_filters:
        os.remove(b2_output)

if  __name__ == '__main__':
    main()
