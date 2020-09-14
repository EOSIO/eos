#!/usr/bin/env python3

import subprocess
import glob

"""
This script benchmarks all operations with all key files
in "data" directory on rocksdb and chainbase.
You can add new key files and their workset files to
"data" directory. a key file must end with ".keys", 
its workset files must start with the key file's main part
and end with ".ws"
"""

def runBenchMarking(key_file, worksets):
    for workset in worksets:
        for db in ["rocksdb", "chainbase"]:
            for op in ["get", "get_data", "set"]:
                cmd = "./benchmark_kv -k " + key_file + " -w " + workset + " -o " + op + " -b " + db
                subprocess.call(cmd.split())

    for db in ["rocksdb", "chainbase"]:
        for op in ["create", "erase", "it_create", "it_next", "it_key_value"]:
            cmd = "./benchmark_kv -k " + key_file + " -o " + op + " -b " + db
            subprocess.call(cmd.split())

def main():
    key_files = glob.glob('data/*.keys')
    for key_file in key_files:
        dot_pos = key_file.find('.')
        name = key_file[:dot_pos]
        workset_files = glob.glob(name+"*.ws")

        runBenchMarking(key_file, workset_files);

if __name__ == "__main__":
    main()
