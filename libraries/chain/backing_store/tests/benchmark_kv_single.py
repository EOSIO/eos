#!/usr/bin/env python3

import sys
import subprocess

"""
This script benchmarks with a single key file and one workset.
It reports both summary and detailed performance numbers
for easy comparison.
"""

def runBenchMarking(key_file, workset):
    output = []
    print("============================================")
    print("Benchmarking starts")
    print("============================================\n")

    for op in ["create", "get", "get_data", "set", "erase", "it_create", "it_next", "it_key", "it_value"]:
        results = []
        for db in ["rocksdb", "chainbase"]:
            cmd = "./benchmark_kv -k " + key_file + " -o " + op + " -b " + db + " -v 512 -s 7"
            if op=="get" or op=="get_data" or op=="set":
                cmd += " -w " + workset

            proc = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            outs,errs= proc.communicate()

            decoded_outs=outs.decode("utf-8")
            print(decoded_outs)
            if errs:
                print(errs.decode("utf-8"))
            results.append(decoded_outs)

        rocksdb = dict(item.split(": ") for item in results[0].split(", "))
        chainbase = dict(item.split(": ") for item in results[1].split(", "))
        out = '{0:<14s}({1:<27} ({2:<}'.format(op, rocksdb["user_cpu_us_avg"] + ", " + rocksdb["system_cpu_us_avg"] + ")", chainbase["user_cpu_us_avg"] + ", " + chainbase["system_cpu_us_avg"] + ")")
        output.append(out)

    print("\nSummary:\n")
    print("Operation,    RocksDB,                     Chainbase")
    print("              (user_cpu_us, sys_cpu_us),   (user_cpu_us, sys_cpu_us)\n")
    for o in output:
        print(o)

def main():
    key_file = sys.argv[1]
    workset = sys.argv[2]
    runBenchMarking(key_file, workset);

if __name__ == "__main__":
    main()
