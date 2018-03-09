#!/usr/bin/env python3

import json
import sys

def json_validator(data):
    try:
        json.loads(data)
        return True
    except ValueError as error:
        print("invalid json: %s" % error)
        return False

def test_json_validator(abi_name):
    abi_file = open(abi_name,'r')
    abi_text = abi_file.read()
    abi_file.close()
    return json_validator(abi_text) == True

if __name__ == "__main__":
    for filename in sys.argv[1:]:
        print("Testing abi file ", filename)
        if test_json_validator(filename):
            exit(0)
        else:
            exit(1)
