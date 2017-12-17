#!/usr/bin/python3

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

###############################################################
# Test for different test scenarios.
# Nodes can be producing or non-producing.
# -p <producing nodes count>
# -n <total nodes>
# -s <topology>
# -d <delay between nodes startup>
###############################################################

DEBUG=True
FNULL = open(os.devnull, 'w')
ADMIN_ACCOUNT="inita"
INITA_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
START_PORT=8888
SYSTEM_BALANCE=0

AccountInfo=namedtuple("AccountInfo", "name ownerPrivate ownerPublic activePrivate activePublic balance")
EosInstanceInfo=namedtuple("EosInstanceInfo", "port")

def killall():
    if 0 != subprocess.call(["programs/launcher/launcher", "-k", "15"]):
        print("ERROR: Launcher failed to shut down eos cluster.")

def cleanup():
    for f in glob.glob("tn_data_*"):
        shutil.rmtree(f)

def errorExit(msg="", errorCode=1):
    print("ERROR:", msg)
    exit(errorCode)

def waitForCluserStability (eosInstanceInfos=[], timeout=60):
    DEBUG and print("Waiting for cluster to stabilize.")
    blockNum=1
    return waitOnBlockSync(blockNum, eosInstanceInfos, timeout)

def waitOnBlockSync (blockNum=1, eosInstanceInfos=[], timeout=60):
    startTime=time.time()
    while time.time()-startTime < timeout:
        stable=True
        for eosInstanceInfo in eosInstanceInfos:
            port=eosInstanceInfo.port
            DEBUG and print("Request block %d from node on port %d" % blockNum, port)
            if 0 != subprocess.call(["programs/eosc/eosc", "--port", str(port), "get", "block", str(blockNum)],
                                        stdout=FNULL, stderr=FNULL):
                DEBUG and print("ERROR: Block %d request failed: " % blockNum)
                stable=False
                break
 
        if stable is True:
            return True
        time.sleep(5)
        print("Waiting on nodes discovery to finish.")

    return False

def createAccountInfos(count):
    accountInfos=[]
    p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
    for i in range(0, count):
        try:
            keyStr=subprocess.check_output(["programs/eosc/eosc", "create", "key"]).decode("utf-8")
            m=p.match(keyStr)
            if m is None:
                print("ERROR: Owner key creation regex mismatch")
                break
            
            ownerPrivate=m.group(1)
            ownerPublic=m.group(2)

            keyStr=subprocess.check_output(["programs/eosc/eosc", "create", "key"]).decode("utf-8")
            m=p.match(keyStr)
            if m is None:
                print("ERROR: Owner key creation regex mismatch")
                break
            
            activePrivate=m.group(1)
            activePublic=m.group(2)

            name=''.join(random.choice(string.ascii_lowercase) for _ in range(5))
            accountInfos.append(AccountInfo(name, ownerPrivate, ownerPublic, activePrivate, activePublic, 0))

        except Exception as e:
            print("ERROR: Exception during key creation:", e)
            break

    if count != len(accountInfos):
        return []

    return accountInfos


def createWallet(INITA_PRV_KEY, accountInfos):

    WALLET_NAME="MyWallet"
    if 0 != subprocess.call(["programs/eosc/eosc", "wallet", "create", "--name", WALLET_NAME], stdout=FNULL):
        print("ERROR: Failed to create wallet.")
        return False

    if 0 != subprocess.call(["programs/eosc/eosc", "wallet", "import", "--name", WALLET_NAME, INITA_PRV_KEY],
                            stdout=FNULL):
        print("ERROR: Failed to import account inita key.")
        return False

    for accountInfo in accountInfos:
        #print("Private: ", key.private)
        if 0 != subprocess.call(["programs/eosc/eosc", "wallet", "import", "--name", WALLET_NAME,
                                 accountInfo.ownerPrivate], stdout=FNULL):
            print("ERROR: Failed to import account owner key.")
            return False

        if 0 != subprocess.call(["programs/eosc/eosc", "wallet", "import", "--name", WALLET_NAME,
                                 accountInfo.activePrivate], stdout=FNULL):
            print("ERROR: Failed to import account active key.")
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
            print("ERROR: Failed to parse head block num.")
            return -1

        s=m.group(1)
        num=int(s)

    except Exception as e:
        print("ERROR: Exception during block number retrieval.", e)
        return -1

    return num

def waitForNextBlock(port=START_PORT):
    num=getHeadBlockNum(port)
    nextNum=num
    while (nextNum <= num):
        time.sleep(0.25)
        nextNum=getHeadBlockNum(port)


def transferFunds(source, destination, amount, port=START_PORT):
    if 0 != subprocess.call(["programs/eosc/eosc", "--port", str(port), "transfer", source, destination,
                             str(amount), "memo"], stdout=FNULL):
        print("ERROR: Failed to transfer funds", amount, "from account "+ source+ " to "+ destination)
        return False

    return True

def spreadFunds(adminAccount, accountInfos, waitNextBlock=True):
    return spreadFunds(1, adminAccount, accountInfos, waitNextBlock):

def spreadFunds(amount=1, adminAccount, accountInfos, waitNextBlock=True):
    if len(accountInfos) == 0:
        return True

    count=len(accountInfos)
    transferAmount=(count*amount)+amount
    port=START_PORT
    fromm=adminAccount
    to=accountInfos[0].name
    print("Transfering %d units from account %s to %s on eos server port %d" % transferAmount, fromm, to, port)
    if False == transferFunds(fromm, to, transferAmount, port):
        return False
    accountInfos[0].balance += transferAmount

    for i in range(0, count):
        if waitNextBlock:
            print("Waiting on next block on eos server port", port)
            waitForNextBlock(port)

        transferAmount -= amount
        fromm=accountInfos[i].name
        to=accountInfos[i+1].name if i < (count-1) else adminAccount
        print("Transfering %d units from account %s to %s on eos server port " % transferAmount, fromm, to, port)

        if False == transferFunds(fromm, to, transferAmount, port):
            return False
        accountInfos[i].balance -= transferAmount
        if i < (count-1):
            accountInfos[i+1].balance += transferAmount
        port += 1

    return True
                              

def validateSpreadFunds(adminAccount, accountInfos, expectedTotal):
    #actualTotal=getAccountBalance(adminAccount)
    actualTotal=SYSTEM_BALANCE
    for accountInfo in accountInfos:
        fund = getAccountBalance(accountInfo.name)
        if fund != accountInfo.balance:
            print("ERROR: validateSpreadFunds> Expected: %d, actual: %d for account %s" % accountInfo.balance, fund, accountInfo.name)
            return False
        actualTotal += fund

    if actualTotal != expectedTotal:
        print("ERROR: validateSpreadFunds> Expected total: %d , actual: %d" % expectedTotal, actualTotal)
        return False
    
    return True

def getSystemBalance(adminAccount, accountInfos)):
    balance=getAccountBalance(source)
    for accountInfo in accountInfos:
        balance += getAccountBalance(accountInfo.name)
    return balance

def spreadFundsAndValidate(amount=1, adminAccount, accountInfos):
    DEBUG and print("Spread Funds and validate")

    initialFunds=getSystemBalance(adminAccount, accountInfos)
    DEBUG and print("Initial system balance: %d" % initialFunds)

    if False == spreadFunds(amount, adminAccount, accountInfos):
        errorExit("Failed to spread funds across nodes.")

    print("Funds spread across all accounts")

    waitForNextBlock()

    if False == validateSpreadFunds(adminAccount, accountInfos, initialFunds):
        errorExit("Failed to validate funds transfer across nodes.")

    print("Funds spread validated")


def getAccountBalance(name):
    try:
        ret=subprocess.check_output(["programs/eosc/eosc", "get", "account", name]).decode("utf-8")
        #print ("getAccountBalance>", ret)
        p=re.compile('\n\s+\"eos_balance\"\:\s+\"(\d+.\d+)\s+EOS\",\n', re.MULTILINE)
        m=p.search(ret)
        if m is None:
            msg="Failed to parse account balance."
            print("ERROR: "+ msg)
            raise Exception(msg)

        s=m.group(1)
        balance=float(s)
        balance *= 10000
        return int(balance)

    except Exception as e:
        print("ERROR: Exception during account balance retrieval.", e)
        raise

    
def createAccount(accountInfo):
    
    if 0 != subprocess.call(["programs/eosc/eosc", "create", "account", ADMIN_ACCOUNT, accountInfo.name, accountInfo.ownerPublic, accountInfo.activePublic], stdout=FNULL):
        print("ERROR: Failed to create inita account.")
        return False

    # wait for next block
    waitForNextBlock()
    
    try :
        ret=subprocess.check_output(["programs/eosc/eosc", "get", "account", accountInfo.name]).decode("utf-8")
        #print ("ret:", ret)
        p=re.compile('\n\s+\"staked_balance\"\:\s+\"\d+.\d+\s+EOS\",\n', re.MULTILINE)
        m=p.search(ret)
        if m is None:
            print("ERROR: Failed to validate account creation.", accountInfo.name)
            return False
    except Exception as e:
        print("ERROR: Exception during account creation validation:", e)
        return False
    
    return True

def getEosInstanceInfos(totalNodes):
    eosInstanceInfos=[]
    for i in range(0, totalNodes):
        eosInstanceInfos.append(EosInstanceInfos(START_PORT + i))
    return eosInstanceInfos
        
    
parser = argparse.ArgumentParser()
parser.add_argument("-p", type=int, help="producing nodes count", default=1)
#parser.add_argument("-n", type=int, help="total nodes", default=0)
parser.add_argument("-d", type=int, help="delay between nodes startup", default=1)
parser.add_argument("-s", type=str, help="topology", default="star")

args = parser.parse_args()
pnodes=args.p
topo=args.s
delay=args.d
#total_nodes = pnodes if args.n == 0 else args.n
total_nodes = pnodes

print ("producing nodes: %d , topology: %s , delay between nodes launch(seconds): %d", pnodes, topo, delay)

cleanup()
random.seed(1) # Use a fixed seed for repeatability.

try:
    launcherOpts="--eosd \"--plugin eosio::wallet_api_plugin\" -p {0} -n {1} -s {2} -d {3}".format(pnodes, total_nodes, topo, delay)
    print ("launcher options:", launcherOpts)

    if 0 != subprocess.call(["programs/launcher/launcher", "--eosd", "--plugin eosio::wallet_api_plugin", "-p", str(pnodes), "-n", str(total_nodes), "-d",
                             str(delay), "-s", topo]):
        errorExit("Launcher failed to stand up eos cluster.")

    eosInstanceInfos=getEosInstanceInfos(total_nodes)

    if not waitForCluserStability(eosInstanceInfos):
        errorExit("Cluster never stabilized")
    print ("Cluster stabilized")

    accountsCount=total_nodes
    accountInfos = createAccountInfos(accountsCount)
    if len(accountInfos) != accountsCount:
        errorExit("Account keys creation failed.")
    print ("Account keys created.")

    SYSTEM_BALANCE=getAccountBalance(ADMIN_ACCOUNT)

    if not createWallet(INITA_PRV_KEY, accountInfos):
        errorExit("Wallet creation failed.")
    print ("Wallet created.")

    for accountInfo in accountInfos:
        if not createAccount(accountInfo):
            errorExit("Account "+ accountInfo.name + " created.")
        print("Account "+ accountInfo.name + " created.")

    # spreadFundsAndValidate(10)
        
    # initialFunds=getAccountBalance(ADMIN_ACCOUNT)
    # print("Initial funds:", initialFunds)

    # if False == spreadFunds(ADMIN_ACCOUNT, accountInfos):
    #     errorExit("Failed to spread funds across nodes.")

    # print("Funds spread across all accounts")

    # waitForNextBlock()

    # if False == validateSpreadFunds(ADMIN_ACCOUNT, accountInfos, initialFunds):
    #     errorExit("Failed to validate funds transfer across nodes.")

    print("Funds spread validated")
finally:
    killall()
    cleanup()
    
exit(0)

