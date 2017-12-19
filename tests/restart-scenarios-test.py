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
import signal
import time
import datetime

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
KILL_PERCENT=.5

AccountInfo=namedtuple("AccountInfo", "name ownerPrivate ownerPublic activePrivate activePublic balance")
EosInstanceInfo=namedtuple("EosInstanceInfo", "port pid cmd alive")
chainStrategy=None

ChainArgs={
    "replay":"--replay-blockchain",
    "resync":"--resync-blockchain"
    }

def killall(eosInstanceInfos):
    if 0 != subprocess.call(["programs/launcher/launcher", "-k", "15"]):
        print("ERROR: Launcher failed to shut down eos cluster.")

    for eosInstanceInfo in eosInstanceInfos:
        try:
            os.kill(eosInstanceInfo.pid, signal.SIGKILL)
        except Exception as e:
            pass
        
def cleanup():
    for f in glob.glob("tn_data_*"):
        shutil.rmtree(f)

def errorExit(msg="", errorCode=1):
    print("ERROR:", msg)
    exit(errorCode)

# def waitForCluserStability (eosInstanceInfos=[], timeout=60):
#     DEBUG and print("Waiting for cluster to stabilize.")
#     blockNum=1
#     return waitOnBlockSync(blockNum, eosInstanceInfos, timeout)

# def waitOnBlockSync (blockNum=1, eosInstanceInfos=[], timeout=60):
#     startTime=time.time()
#     while time.time()-startTime < timeout:
#         stable=True
#         for eosInstanceInfo in eosInstanceInfos:
#             port=eosInstanceInfo.port
#             DEBUG and print("Request block %d from node on port %d" % (blockNum, port))
#             if 0 != subprocess.call(["programs/eosc/eosc", "--port", str(port), "get", "block", str(blockNum)],
#                                         stdout=FNULL, stderr=FNULL):
#                 DEBUG and print("ERROR: Block %d request failed: " % (blockNum))
#                 stable=False
#                 break
 
#         if stable is True:
#             return True
#         time.sleep(5)
#         print("Waiting on nodes discovery to finish.")

#     return False

def waitOnClusterSync(eosInstanceInfos, timeout=60):
    startTime=time.time()
    if eosInstanceInfos[0].alive is False:
        print("ERROR: Root node is down.")
        return False;
    targetHeadBlockNum=getHeadBlockNum(eosInstanceInfos[0]) #get root nodes head block num
    if targetHeadBlockNum == -1:
        return False
    while time.time()-startTime < timeout:
        synced=True
        for eosInstanceInfo in eosInstanceInfos:
            if eosInstanceInfo.alive:
                port=eosInstanceInfo.port
                DEBUG and print("Request block %d from node on port %d" % (targetHeadBlockNum, port))
                if 0 != subprocess.call(["programs/eosc/eosc", "--port", str(port), "get", "block", str(targetHeadBlockNum)],
                                        stdout=FNULL, stderr=FNULL):
                    DEBUG and print("ERROR: Block %d request failed: " % (targetHeadBlockNum))
                    synced=False
                    break

        if synced is True:
            return True
        time.sleep(5)
        print("Waiting on nodes to synchronize.")

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


def getHeadBlockNum(eosInstanceInfo):
    port=eosInstanceInfo.port
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

def waitForNextBlock(eosInstanceInfo):
    num=getHeadBlockNum(eosInstanceInfo)
    nextNum=num
    while (nextNum <= num):
        time.sleep(0.25)
        nextNum=getHeadBlockNum(eosInstanceInfo)


def transferFunds(source, destination, amount, port=START_PORT):
    if 0 != subprocess.call(["programs/eosc/eosc", "--port", str(port), "transfer", source, destination,
                             str(amount), "memo"], stdout=FNULL):
        print("ERROR: Failed to transfer funds", amount, "from account "+ source+ " to "+ destination)
        return False

    return True

def spreadFunds(adminAccount, accountInfos, eosInstanceInfos, waitNextBlock=True):
    return spreadFunds(adminAccount, accountInfos, eosInstanceInfos, 1, waitNextBlock)

# TBD: Extend to skip dead nodes
def spreadFunds(adminAccount, accountInfos, eosInstanceInfos, amount=1, waitNextBlock=True):
    if len(accountInfos) == 0:
        return True

    count=len(accountInfos)
    transferAmount=(count*amount)+amount
    port=eosInstanceInfos[0].port
    fromm=adminAccount
    to=accountInfos[0].name
    print("Transfering %d units from account %s to %s on eos server port %d" % (transferAmount, fromm, to, port))
    if False == transferFunds(fromm, to, transferAmount, port):
        return False
    newBalance = accountInfos[0].balance + transferAmount
    accountInfos[0] = accountInfos[0]._replace(balance=newBalance);

    for i in range(0, count):
        port=eosInstanceInfos[i].port
        if waitNextBlock:
            print("Waiting on next block on eos server port", port)
            waitForNextBlock(eosInstanceInfos[i])

        transferAmount -= amount
        fromm=accountInfos[i].name
        to=accountInfos[i+1].name if i < (count-1) else adminAccount
        print("Transfering %d units from account %s to %s on eos server port %d." % (transferAmount, fromm, to, port))

        if False == transferFunds(fromm, to, transferAmount, port):
            return False
        newBalance = accountInfos[i].balance - transferAmount
        accountInfos[i] = accountInfos[i]._replace(balance=newBalance);
        if i < (count-1):
            newBalance = accountInfos[i+1].balance + transferAmount
            accountInfos[i+1] = accountInfos[i+1]._replace(balance=newBalance);

    return True
                              

def validateSpreadFunds(adminAccount, accountInfos, expectedTotal):
    actualTotal=getAccountBalance(adminAccount)
    for accountInfo in accountInfos:
        fund = getAccountBalance(accountInfo.name)
        if fund != accountInfo.balance:
            print("ERROR: validateSpreadFunds> Expected: %d, actual: %d for account %s" % (accountInfo.balance, fund, accountInfo.name))
            return False
        actualTotal += fund

    if actualTotal != expectedTotal:
        print("ERROR: validateSpreadFunds> Expected total: %d , actual: %d" % (expectedTotal, actualTotal))
        return False
    
    return True

def getSystemBalance(adminAccount, accountInfos):
    balance=getAccountBalance(adminAccount)
    for accountInfo in accountInfos:
        balance += getAccountBalance(accountInfo.name)
    return balance

def spreadFundsAndValidate(adminAccount, accountInfos, eosInstanceInfos, amount=1):
    DEBUG and print("Spread Funds and validate")

    initialFunds=getSystemBalance(adminAccount, accountInfos)
    DEBUG and print("Initial system balance: %d" % (initialFunds))

    if False == spreadFunds(adminAccount, accountInfos, eosInstanceInfos, amount):
        errorExit("Failed to spread funds across nodes.")

    print("Funds spread across all accounts")

    waitForNextBlock(eosInstanceInfos[0])

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

    
def createAccount(accountInfo, eosInstanceInfo):
    
    if 0 != subprocess.call(["programs/eosc/eosc", "create", "account", ADMIN_ACCOUNT, accountInfo.name, accountInfo.ownerPublic, accountInfo.activePublic], stdout=FNULL):
        print("ERROR: Failed to create inita account.")
        return False

    # wait for next block
    waitForNextBlock(eosInstanceInfo)
    
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

# Populates list of EosInstanceInfo objects, matched to actuall running instances
def getEosInstanceInfos(totalNodes):
    eosInstanceInfos=[]

    try:
        psOut=subprocess.check_output(["pgrep", "-a", "eosd"]).decode("utf-8")
        print("psOut: <%s>" % psOut)

        for i in range(0, totalNodes):
            pattern="[\n]?(\d+) (.* --data-dir tn_data_%02d)\n" % (i)
            m=re.search(pattern, psOut, re.MULTILINE)
            if m is None:
                print("ERROR: Failed to find eosd pid. Pattern %s" % (pattern))
                break
            instance=EosInstanceInfo(START_PORT + i, int(m.group(1)), m.group(2), True)
            print("EosInstanceInfo:", instance)
            eosInstanceInfos.append(instance)
            
    except Exception as ex:
        print("ERROR: Exception during EosInstanceInfos creation:", e)
        
    return eosInstanceInfos

# Check state of running eosd process and update EosInstanceInfos
def updateEosInstanceInfos(eosInstanceInfos):
    for i in range(0, len(eosInstanceInfos)):
        eosInstanceInfo=eosInstanceInfos[i]
        running=True
        try:
            os.kill(eosInstanceInfo.pid, 0)
        except Exception as ex:
            running=False
        if running != eosInstanceInfo.alive:
            eosInstanceInfos[i]=eosInstanceInfo._replace(alive=running)

    return eosInstanceInfos

# Kills a percentange of Eos instances starting from the tail and update eosInstanceInfos state
def killSomeEosInstances(eosInstanceInfos, percent):
    killCount=int(percent*len(eosInstanceInfos))
    DEBUG and print("Killing %d Eosd instances." % (killCount))

    killedCount=0
    for eosInstanceInfo in reversed(eosInstanceInfos):
        try:
            os.kill(eosInstanceInfo.pid, signal.SIGTERM)
        except Exception as ex:
            print("ERROR: Failed to kill process pid %d." % (eosInstanceInfo.pid), ex)
            return False
        killedCount += 1
        if killedCount >= killCount:
            break

    time.sleep(1) # Give processes time to stand down
    return updateEosInstanceInfos(eosInstanceInfos)

def relaunchEosInstances(eosInstanceInfos):

    chainArg=ChainArgs.get(chainStrategy)

    for i in range(0, len(eosInstanceInfos)):
        eosInstanceInfo=eosInstanceInfos[i]
        running=True
        try:
            os.kill(eosInstanceInfo.pid, 0) #check if instance is running
        except Exception as ex:
            running=False

        if running is False:
            dataDir="tn_data_%02d" % (i)
            dt = datetime.datetime.now()
            dateStr="%d_%02d_%02d_%02d_%02d_%02d" % (dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)
            stdoutFile="%s/stdout.%s.txt" % (dataDir, dateStr)
            stderrFile="%s/stderr.%s.txt" % (dataDir, dateStr)
            with open(stdoutFile, 'w') as sout, open(stderrFile, 'w') as serr:
                cmd=eosInstanceInfo.cmd + ("" if chainArg is None else (" " + chainArg))
                print("Eosd cmd: " + cmd)
                popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
                eosInstanceInfos[i]=eosInstanceInfo._replace(pid=popen.pid)

    updateEosInstanceInfos(eosInstanceInfos)
    return True

def launcheCluster(pnodes, total_nodes, topo, delay):
    launcherOpts="--eosd \"--plugin eosio::wallet_api_plugin\" -p {0} -n {1} -s {2} -d {3}".format(pnodes, total_nodes, topo, delay)
    DEBUG and print ("launcher options:", launcherOpts)

    if 0 != subprocess.call(["programs/launcher/launcher", "--eosd", "--plugin eosio::wallet_api_plugin", "-p", str(pnodes), "-n", str(total_nodes), "-d",
                             str(delay), "-s", topo]):
        print("ERROR: Launcher failed to launch.")
        return None

    eosInstanceInfos=getEosInstanceInfos(total_nodes)
    return eosInstanceInfos

def createAccounts(accountInfos, eosInstanceInfo):
    for accountInfo in accountInfos:
        if not createAccount(accountInfo, eosInstanceInfo):
            print("ERROR: Failed to create account %s." % (accountInfo.name))
            return False
        DEBUG and print("Account %s created." % (accountInfo.name))

    return True

def killRelaunchAndResync(eosInstanceInfos, KILL_PERCENT):
    DEBUG and print("Killing %.2f%% cluster node instances." % (KILL_PERCENT))
    if killSomeEosInstances(eosInstanceInfos, KILL_PERCENT) is False:
        print("ERROR: Failed to kill Eos instances")
        return False
    DEBUG and print("Eosd instances killed.")
    

    return True

parser = argparse.ArgumentParser()
parser.add_argument("-p", type=int, help="producing nodes count", default=1)
parser.add_argument("-d", type=int, help="delay between nodes startup", default=1)
parser.add_argument("-s", type=str, help="topology", default="star")
parser.add_argument("-c", type=str, help="chain strategy[replay|resync|none]", default="none")

args = parser.parse_args()
pnodes=args.p
topo=args.s
delay=args.d
chainStrategy=args.c
total_nodes = pnodes

print ("producing nodes: %d , topology: %s , delay between nodes launch(seconds): %d" % (pnodes, topo, delay))

cleanup()
random.seed(1) # Use a fixed seed for repeatability.

eosInstanceInfos=None
try:
    eosInstanceInfos=launcheCluster(pnodes, total_nodes, topo, delay)
    if None == eosInstanceInfos:
        errorExit("Failed to stand up eos cluster.")

    if total_nodes != len(eosInstanceInfos):
        errorExit("ERROR: Unable to validate eosd instances, expected: %d, actual: %d" % (total_nodes, len(eosInstanceInfos)))

        
    #if not waitForCluserStability(eosInstanceInfos):
    if not waitOnClusterSync(eosInstanceInfos):
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

    if not createAccounts(accountInfos, eosInstanceInfos[0]):
        errorExit("Accounts creation failed.")
    print("Accounts created.")

    spreadFundsAndValidate(ADMIN_ACCOUNT, accountInfos, eosInstanceInfos, 10)
    print("Funds spread validated")

    print("Killing %.2f%% cluster node instances." % (KILL_PERCENT))
    if killSomeEosInstances(eosInstanceInfos, KILL_PERCENT) is False:
        errorExit("Failed to kill Eos instances")
    print("Eosd instances killed.")

    # spreadFundsAndValidate(ADMIN_ACCOUNT, accountInfos, eosInstanceInfos, 10)
    # print("Funds spread validated")

    print ("Relaunching dead cluster nodes instances.")
    if relaunchEosInstances(eosInstanceInfos) is False:
        errorExit("Failed to relaunch Eos instances")
    print("Eosd instances relaunched.")

    print ("Resyncing cluster nodes.")
    if not waitOnClusterSync(eosInstanceInfos):
        errorExit("Cluster never synchronized")
    print ("Cluster resynched")

    spreadFundsAndValidate(ADMIN_ACCOUNT, accountInfos, eosInstanceInfos, 10)
    print("Funds spread validated")
    
finally:
    killall(eosInstanceInfos)
    cleanup()
    pass
    
exit(0)
