#!/usr/bin/python

import argparse
import subprocess
import time
import glob
import shutil
import time
import os
from collections import namedtuple
import re

def killall():
    subprocess.call(["programs/launcher/launcher", "-k", "15"])

def cleanup():
    for f in glob.glob("tn_data_*"):
        shutil.rmtree(f)

def errorExit(msg="", errorCode=1):
    print(msg)
    killall()
    exit(errorCode)

def waitForCluserStability (ports=[], timeout=60):

    startTime=time.time()
    while time.time()-startTime < timeout:
        stable=True
        for port in ports:
            print("Request block 1 from node on port", port)
            try:
                subprocess.check_output(["programs/eosc/eosc", "--port", str(port), "get", "block", "1"],
                                        stderr=subprocess.STDOUT)
            except Exception as e:
                print ("Block 1 request failed: ", e)
                stable=False
                break

        if stable is True:
            return True
        time.sleep(5)

    return False

Key=namedtuple("Key", "private public")

def createKeys(count):
    keys=[]
    p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
    for i in range(0, count):
        try:
            keyStr=subprocess.check_output(["programs/eosc/eosc", "create", "key"]).decode("utf-8")
            m=p.match(keyStr)
            if m is not None:
                private=m.group(1)
                public=m.group(2)
                keys.append(Key(private, public))

        except Exception as e:
            print("Exception during key creation:", e)
            break

    return keys

def createAccounts(INITA_PRV_KEY, keys):

    FNULL = open(os.devnull, 'w')
    if 0 != subprocess.run(["programs/eosc/eosc", "wallet", "create", "--name", "inita"]):
        print("Failed to create inita account.")
        return False

    if 0 != subprocess.run(["programs/eosc/eosc", "wallet", "import", "--name", "inita", INITA_PRV_KEY]):
        print("Failed to import account inita key.")
        return False

    for key in keys:
        print("Private: ", key.private)
    # if 0 != subprocess.run(["programs/eosc/eosc", "wallet", "import", "--name", "inita", INITA_PRV_KEY]):
    #     print("Failed to import account inita key.")
    #     return False


    return True

# createAccounts("1", "2")
# exit(0)

parser = argparse.ArgumentParser()

parser.add_argument("-p", type=int, help="producing nodes count", default=1)
parser.add_argument("-n", type=int, help="total nodes", default=0)
parser.add_argument("-d", type=int, help="delay between nodes startup", default=1)
parser.add_argument("-s", type=str, help="topology", default="star")

args = parser.parse_args()
pnodes=args.p
topo=args.s
delay=args.d
total_nodes = pnodes if args.n == 0 else args.n


print ("pnodes:", pnodes, ", total_nodes: ", total_nodes, ", topo:", topo, ", delay:", delay)

INITA_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"

cleanup()

launcherOpts="-p {0} -n {1} -s {2} -d {3}".format(pnodes, total_nodes, topo, delay)
print ("launcher options:", launcherOpts)

subprocess.call(["programs/launcher/launcher", "-p", str(pnodes), "-n", str(total_nodes), "-d",  str(delay), "-s", topo])
#time.sleep(7)

ports=[]
for i in range(0, total_nodes):
    ports.append(8888 + i)

if not waitForCluserStability(ports):
    errorExit(msg="Cluster never stabilized")
print ("Cluster stabalized")

accountsCount=3
keys = createKeys(accountsCount)
if len(keys) != accountsCount:
    errorExit(msg="Key creation failed.")

if not createAccounts(INITA_PRV_KEY, keys):
    errorExit(msg="Accounts creation failed.")

killall()
cleanup()
