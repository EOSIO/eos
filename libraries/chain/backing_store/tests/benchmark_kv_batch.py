#!/usr/bin/env python3

import subprocess
import glob

from benchmark_kv_single import runBenchMarking

"""
This script benchmarks all operations with all key files
in "data" directory on rocksdb and chainbase.
You can add new key files and their workset files to
"data" directory. a key file must end with ".keys", 
its workset files must start with the key file's main part
and end with ".ws"
A sample data directory would look like this
10m_random_1k.ws  5m_random_1k.ws  5m_random.keys   5m_sorted_5k.ws
10m_random.keys   5m_random_5k.ws  5m_sorted_1k.ws  5m_sorted.keys
"""
def main():
    key_files = glob.glob('data/*.keys')
    for key_file in key_files:
        dot_pos = key_file.find('.')
        name = key_file[:dot_pos]
        workset_files = glob.glob(name+"*.ws")

        for workset in workset_files:
            runBenchMarking(key_file, workset);

if __name__ == "__main__":
    main()
