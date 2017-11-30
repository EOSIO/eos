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
import random
import string

DEBUG=False
FNULL = open(os.devnull, 'w')
ADMIN_ACCOUNT="inita"

def killall():
    subprocess.call(["programs/launcher/launcher", "-k", "15"])

def cleanup():
    for f in glob.glob("tn_data_*"):
        shutil.rmtree(f)

def errorExit(msg="", errorCode=1):
    print("ERROR:", msg)
    #killall()
    exit(errorCode)

def waitForCluserStability (ports=[], timeout=60):

    startTime=time.time()
    while time.time()-startTime < timeout:
        stable=True
        for port in ports:
            DEBUG and print("Request block 1 from node on port", port)
            if 0 != subprocess.call(["programs/eosc/eosc", "--port", str(port), "get", "block", "1"],
                                        stdout=FNULL, stderr=FNULL):
                DEBUG and print("Block 1 request failed: ", e)
                stable=False
                break
 
        if stable is True:
            return True
        time.sleep(5)
        print("Waiting on nodes discovery to finish.")

    return False

AccountInfo=namedtuple("AccountInfo", "name ownerPrivate ownerPublic activePrivate activePublic")

def createAccountInfos(count):
    accountInfos=[]
    p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
    for i in range(0, count):
        try:
            keyStr=subprocess.check_output(["programs/eosc/eosc", "create", "key"]).decode("utf-8")
            m=p.match(keyStr)
            if m is None:
                print("Owner key creation regex mismatch")
                break
            
            ownerPrivate=m.group(1)
            ownerPublic=m.group(2)

            keyStr=subprocess.check_output(["programs/eosc/eosc", "create", "key"]).decode("utf-8")
            m=p.match(keyStr)
            if m is None:
                print("Owner key creation regex mismatch")
                break
            
            activePrivate=m.group(1)
            activePublic=m.group(2)

            name=''.join(random.choice(string.ascii_lowercase) for _ in range(5))
            accountInfos.append(AccountInfo(name, ownerPrivate, ownerPublic, activePrivate, activePublic))

        except Exception as e:
            print("Exception during key creation:", e)
            break

    return accountInfos


def createWallet(INITA_PRV_KEY, accountInfos):

    if 0 != subprocess.call(["programs/eosc/eosc", "wallet", "create", "--name", ADMIN_ACCOUNT], stdout=FNULL):
        print("Failed to create inita account.")
        return False

    if 0 != subprocess.call(["programs/eosc/eosc", "wallet", "import", "--name", ADMIN_ACCOUNT, INITA_PRV_KEY],
                            stdout=FNULL):
        print("Failed to import account inita key.")
        return False

    for accountInfo in accountInfos:
        #print("Private: ", key.private)
        if 0 != subprocess.call(["programs/eosc/eosc", "wallet", "import", "--name", ADMIN_ACCOUNT,
                                 accountInfo.ownerPrivate], stdout=FNULL):
            print("Failed to import account owner key.")
            return False

        if 0 != subprocess.call(["programs/eosc/eosc", "wallet", "import", "--name", ADMIN_ACCOUNT,
                                 accountInfo.activePrivate], stdout=FNULL):
            print("Failed to import account active key.")
            return False

    return True


def getHeadBlockNum(port):
    num=-1
    try:
        ret=subprocess.check_output(["programs/eosc/eosc", "--port", str(port), "get", "info"]).decode("utf-8")
        #print("eosc get info: ", ret)
        p=re.compile('\n\s+\"head_block_num\"\:\s+(\d+),\n', re.MULTILINE)
        m=p.search(ret)
        if m is None:
            print("Failed to parse head block num.")
            return -1

        s=m.group(1)
        num=int(s)

    except Exception as e:
        print("Exception during block number retrieval.", e)
        return -1

    return num

def waitForNextBlock(port=8888):
    num=getHeadBlockNum(port)
    nextNum=num
    while (nextNum <= num):
        time.sleep(0.25)
        nextNum=getHeadBlockNum(port)


def transferFunds(source, destination, amount, port=8888):
    if 0 != subprocess.call(["programs/eosc/eosc", "--port", str(port), "transfer", source, destination,
                             str(amount), "memo"], stdout=FNULL):
        print("Failed to transfer funds", amount, "from account "+ source+ " to "+ destination)
        return False

    return True


def spreadFunds(source, accountInfos, waitNextBlock=True):
    if len(accountInfos) == 0:
        return True

    count=len(accountInfos)
    transferAmount=count+1
    port=8888
    fromm=source
    to=accountInfos[0].name
    print("Transfering", transferAmount, "units from account "+ fromm+ " to "+ to+ " on eos server port ", port)
    if False == transferFunds(fromm, to, transferAmount, port):
        return False

    for i in range(0, count):
        if waitNextBlock:
            print("Waiting on next block on eos server port", port)
            waitForNextBlock(port)

        transferAmount -= 1
        #currentName=accountInfos[i].name
        #nextName= accountInfos[i+1].name if i < (count-1) else source
        fromm=accountInfos[i].name
        to=accountInfos[i+1].name if i < (count-1) else source
        print("Transfering", transferAmount, "units from account "+ fromm+ " to "+ to+ " on eos server port ",
              port)

        if False == transferFunds(fromm, to, transferAmount, port):
            return False
        port += 1

    return True
                              

def validateSpreadFunds(source, accountInfos, expectedTotal):
    actualTotal=getAccountBalance(source)
    for accountInfo in accountInfos:
        fund = getAccountBalance(accountInfo.name)
        if fund != 1:
            print("ERROR: validateSpreadFunds> Expected 1, actual:", fund, "for account "+ accountInfo.name)
            return False
        actualTotal += fund

    if actualTotal != expectedTotal:
            print("ERROR: validateSpreadFunds> Expected total", expectedTotal, ", actual:", actualTotal)
            return False
    
    return True


def getAccountBalance(name):
    try:
        ret=subprocess.check_output(["programs/eosc/eosc", "get", "account", name]).decode("utf-8")
        #print ("getAccountBalance>", ret)
        p=re.compile('\n\s+\"eos_balance\"\:\s+\"(\d+.\d+)\s+EOS\",\n', re.MULTILINE)
        m=p.search(ret)
        if m is None:
            msg="Failed to parse account balance."
            print(msg)
            raise Exception(msg)

        s=m.group(1)
        balance=float(s)
        balance *= 10000
        return int(balance)

    except Exception as e:
        print("Exception during account balance retrieval.", e)
        raise

    
def createAccount(accountInfo):
    
    if 0 != subprocess.call(["programs/eosc/eosc", "create", "account", ADMIN_ACCOUNT, accountInfo.name, accountInfo.ownerPublic, accountInfo.activePublic], stdout=FNULL):
        print("Failed to create inita account.")
        return False

    # wait for next block
    waitForNextBlock()
    
    try :
        ret=subprocess.check_output(["programs/eosc/eosc", "get", "account", accountInfo.name]).decode("utf-8")
        #print ("ret:", ret)
        p=re.compile('\n\s+\"staked_balance\"\:\s+\"\d+.\d+\s+EOS\",\n', re.MULTILINE)
        m=p.search(ret)
        if m is None:
            print("Failed to validate account creation.", accountInfo.name)
            return False
    except Exception as e:
        print("Exception during account creation validation:", e)
        return False
    
    return True
    

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


print ("producing nodes:", pnodes, ", non-producing nodes: ", total_nodes-pnodes,
       ", topology:", topo, ", delay between nodes launch(seconds):", delay)

INITA_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"

cleanup()

try:
    launcherOpts="-p {0} -n {1} -s {2} -d {3}".format(pnodes, total_nodes, topo, delay)
    print ("launcher options:", launcherOpts)

    subprocess.call(["programs/launcher/launcher", "-p", str(pnodes), "-n", str(total_nodes), "-d",
                     str(delay), "-s", topo])

    ports=[]
    for i in range(0, total_nodes):
        ports.append(8888 + i)

    if not waitForCluserStability(ports):
        errorExit("Cluster never stabilized")
    print ("Cluster stabalized")

    accountsCount=total_nodes
    accountInfos = createAccountInfos(accountsCount)
    if len(accountInfos) != accountsCount:
        errorExit("Account keys creation failed.")
    print ("Account keys created.")

    if not createWallet(INITA_PRV_KEY, accountInfos):
        errorExit("Wallet creation failed.")
    print ("Wallet created.")

    for accountInfo in accountInfos:
        if not createAccount(accountInfo):
            errorExit("Account "+ accountInfo.name + " created.")

        print("Account "+ accountInfo.name + " created.")

    initialFunds=getAccountBalance(ADMIN_ACCOUNT)
    print("Initial funds:", initialFunds)

    if False == spreadFunds(ADMIN_ACCOUNT, accountInfos):
        errorExit("Failed to spread funds across nodes.")

    print("Funds spread across all accounts")

    waitForNextBlock()

    if False == validateSpreadFunds(ADMIN_ACCOUNT, accountInfos, initialFunds):
        errorExit("Failed to validate funds transfer across nodes.")

    print("Funds spread validated")
finally:
    killall()
    cleanup()
    
exit(0)

