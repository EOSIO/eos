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
import inspect
import sys

###############################################################
# Test for different test scenarios.
# Nodes can be producing or non-producing.
# -p <producing nodes count>
# -c <chain strategy[replay|resync|none]>
# -s <topology>
# -d <delay between nodes startup>
###############################################################

DEBUG=False
FNULL = open(os.devnull, 'w')
ADMIN_ACCOUNT="inita"
INITA_PRV_KEY="5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"
START_PORT=8888
SYSTEM_BALANCE=0
KILL_PERCENT=50

AccountInfo=namedtuple("AccountInfo", "name ownerPrivate ownerPublic activePrivate activePublic balance")
EosInstanceInfo=namedtuple("EosInstanceInfo", "port pid cmd alive")
SyncStrategy=namedtuple("ChainSyncStrategy", "name id arg")
chainSyncStrategies={}
chainSyncStrategy=None

def myPrint(*args, **kwargs):
    stackDepth=len(inspect.stack())-2
    str=' '*stackDepth
    sys.stdout.write(str)
    print(*args, **kwargs)

def initializeChainStrategies():
    chainSyncStrategy=SyncStrategy("none", 0, "")
    chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

    chainSyncStrategy=SyncStrategy("replay", 1, "--replay-blockchain")
    chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

    chainSyncStrategy=SyncStrategy("resync", 2, "--resync-blockchain")
    chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

def killall(eosInstanceInfos):
    cmd="programs/launcher/launcher -k 15"
    DEBUG and myPrint("cmd: %s" % (cmd))
    if 0 != subprocess.call(cmd.split(), stdout=FNULL):
        myPrint("Launcher failed to shut down eos cluster.")

    for eosInstanceInfo in eosInstanceInfos:
        try:
            os.kill(eosInstanceInfo.pid, signal.SIGKILL)
        except Exception as ex:
            pass
        
def cleanup():
    for f in glob.glob("tn_data_*"):
        shutil.rmtree(f)

def errorExit(msg="", errorCode=1):
    myPrint("ERROR:", msg)
    exit(errorCode)

def waitOnClusterSync(eosInstanceInfos, timeout=60):
    startTime=time.time()
    if eosInstanceInfos[0].alive is False:
        myPrint("ERROR: Root node is down.")
        return False;
    targetHeadBlockNum=getHeadBlockNum(eosInstanceInfos[0]) #get root nodes head block num
    DEBUG and myPrint("Head block number on root node: %d" % (targetHeadBlockNum))
    if targetHeadBlockNum == -1:
        return False

    currentTimeout=timeout-(time.time()-startTime)
    return waitOnClusterBlockNumSync(eosInstanceInfos, targetHeadBlockNum, currentTimeout)

def waitOnClusterBlockNumSync(eosInstanceInfos, targetHeadBlockNum, timeout=60):
    startTime=time.time()
    remainingTime=timeout
    while time.time()-startTime < timeout:
        synced=True
        for eosInstanceInfo in eosInstanceInfos:
            if eosInstanceInfo.alive:
                if doesNodeHaveBlockNum(targetHeadBlockNum, eosInstanceInfo) is False:
                    synced=False
                    break

        if synced is True:
            return True
        #DEBUG and myPrint("Brief pause to allow nodes to catch up.")
        sleepTime=3 if remainingTime > 3 else (3 - remainingTime)
        remainingTime -= sleepTime
        time.sleep(sleepTime)

    return False

def doesNodeHaveBlockNum(blockNum, eosInstanceInfo):
    if eosInstanceInfo.alive is False:
        return False
    
    port=eosInstanceInfo.port
    DEBUG and myPrint("Request block num %s from node on port %d" % (blockNum, port))
    cmd="programs/eosc/eosc --port %d get block %s" % (port, blockNum)
    DEBUG and myPrint("cmd: %s" % (cmd))
    if 0 != subprocess.call(cmd.split(), stdout=FNULL, stderr=FNULL):
        DEBUG and myPrint("Block num %s request failed: " % (blockNum))
        return False

    return True

def doesNodeHaveTransId(transId, eosInstanceInfo):
    if eosInstanceInfo.alive is False:
        return False
    
    port=eosInstanceInfo.port
    DEBUG and myPrint("Request transation id %s from node on port %d" % (transId, port))
    cmd="programs/eosc/eosc --port %d get transaction %s" % (port, transId)
    DEBUG and myPrint("cmd: %s" % (cmd))
    if 0 != subprocess.call(cmd.split(), stdout=FNULL, stderr=FNULL):
        DEBUG and myPrint("Transaction id %s request failed: " % (transId))
        return False

    return True

def waitForBlockNumOnNode(blockNum, eosInstanceInfo, timeout=60):
    startTime=time.time()
    remainingTime=timeout
    while time.time()-startTime < timeout:
        if doesNodeHaveBlockNum(blockNum, eosInstanceInfo):
            return True
        sleepTime=3 if remainingTime > 3 else (3 - remainingTime)
        remainingTime -= sleepTime
        time.sleep(sleepTime)

    return False

def waitForTransIdOnNode(transId, eosInstanceInfo, timeout=60):
    startTime=time.time()
    remainingTime=timeout
    while time.time()-startTime < timeout:
        if doesNodeHaveTransId(transId, eosInstanceInfo):
            return True
        sleepTime=3 if remainingTime > 3 else (3 - remainingTime)
        remainingTime -= sleepTime
        time.sleep(sleepTime)

    return False
    

def createAccountInfos(count):
    accountInfos=[]
    p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
    for i in range(0, count):
        try:
            cmd="programs/eosc/eosc create key"
            DEBUG and myPrint("cmd: %s" % (cmd))
            keyStr=subprocess.check_output(cmd.split()).decode("utf-8")
            m=p.match(keyStr)
            if m is None:
                myPrint("ERROR: Owner key creation regex mismatch")
                break
            
            ownerPrivate=m.group(1)
            ownerPublic=m.group(2)

            cmd="programs/eosc/eosc create key"
            DEBUG and myPrint("cmd: %s" % (cmd))
            keyStr=subprocess.check_output(cmd.split()).decode("utf-8")
            m=p.match(keyStr)
            if m is None:
                myPrint("ERROR: Owner key creation regex mismatch")
                break
            
            activePrivate=m.group(1)
            activePublic=m.group(2)

            name=''.join(random.choice(string.ascii_lowercase) for _ in range(5))
            accountInfos.append(AccountInfo(name, ownerPrivate, ownerPublic, activePrivate, activePublic, 0))

        except Exception as ex:
            myPrint("ERROR: Exception during key creation:", ex)
            break

    if count != len(accountInfos):
        return []

    return accountInfos


def createWallet(INITA_PRV_KEY, accountInfos):

    WALLET_NAME="MyWallet"
    cmd="programs/eosc/eosc wallet create --name %s" % (WALLET_NAME)
    DEBUG and myPrint("cmd: %s" % (cmd))
    if 0 != subprocess.call(cmd.split(), stdout=FNULL):
        myPrint("ERROR: Failed to create wallet.")
        return False

    cmd="programs/eosc/eosc wallet import --name %s %s" % (WALLET_NAME, INITA_PRV_KEY)
    DEBUG and myPrint("cmd: %s" % (cmd))
    if 0 != subprocess.call(cmd.split(),
                            stdout=FNULL):
        myPrint("ERROR: Failed to import account inita key.")
        return False

    for accountInfo in accountInfos:
        #myPrint("Private: ", key.private)
        cmd="programs/eosc/eosc wallet import --name %s %s" % (WALLET_NAME, accountInfo.ownerPrivate)
        DEBUG and myPrint("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=FNULL):
            myPrint("ERROR: Failed to import account owner key.")
            return False

        cmd="programs/eosc/eosc wallet import --name %s %s" % (WALLET_NAME, accountInfo.activePrivate)
        DEBUG and myPrint("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=FNULL):
            myPrint("ERROR: Failed to import account active key.")
            return False

    return True

def getHeadBlockNum(eosInstanceInfo):
    port=eosInstanceInfo.port
    num=-1
    try:
        cmd="programs/eosc/eosc --port %d get info" % port
        DEBUG and myPrint("cmd: %s" % (cmd))
        ret=subprocess.check_output(cmd.split()).decode("utf-8")
        #myPrint("eosc get info: ", ret)
        p=re.compile('\n\s+\"head_block_num\"\:\s+(\d+),\n', re.MULTILINE)
        m=p.search(ret)
        if m is None:
            myPrint("ERROR: Failed to parse head block num.")
            return -1

        s=m.group(1)
        num=int(s)

    except Exception as ex:
        myPrint("ERROR: Exception during block number retrieval.", ex)
        return -1

    return num

def waitForNextBlock(eosInstanceInfo):
    num=getHeadBlockNum(eosInstanceInfo)
    nextNum=num
    while (nextNum <= num):
        time.sleep(0.50)
        nextNum=getHeadBlockNum(eosInstanceInfo)

# returns transaction id # for this transaction
def transferFunds(source, destination, amount, eosInstanceInfo):
    trans=None
    port=eosInstanceInfo.port
    p = re.compile('\n\s+\"transaction_id\":\s+\"(\w+)\",\n', re.MULTILINE)
    cmd="programs/eosc/eosc --port %d transfer %s %s %d memo" % (port, source, destination, amount)
    DEBUG and myPrint("cmd: %s" % (cmd))
    try:
        retStr=subprocess.check_output(cmd.split()).decode("utf-8")
        #myPrint("Ret: ", retStr)
        m=p.search(retStr)
        if m is None:
           myPrint("ERROR: transaction id parser failure")
           return None
        transId=m.group(1)
        #myPrint("Transaction block num: %s" % (blockNum))
       
    except Exception as ex:
        myPrint("ERROR: Exception during funds transfer.", ex)
        return None

    return transId

def spreadFunds(adminAccount, accountInfos, eosInstanceInfos):
    return spreadFunds(adminAccount, accountInfos, eosInstanceInfos, 1)

# Spread funds across accounts with transactions spread through cluster nodes.
#  Validate transactions are synchronized on root node
def spreadFunds(adminAccount, accountInfos, eosInstanceInfos, amount=1):
    if len(accountInfos) == 0:
        return True

    count=len(accountInfos)
    transferAmount=(count*amount)+amount
    eosInstanceInfo=eosInstanceInfos[0]
    port=eosInstanceInfo.port
    fromm=adminAccount
    to=accountInfos[0].name
    myPrint("Transfer %d units from account %s to %s on eos server port %d" % (transferAmount, fromm, to, port))
    transId=transferFunds(fromm, to, transferAmount, eosInstanceInfo)
    if transId is None:
        return False
    DEBUG and myPrint("Funds transfered on transaction id %s." % (transId))
    newBalance = accountInfos[0].balance + transferAmount
    accountInfos[0] = accountInfos[0]._replace(balance=newBalance);

    nextEosIdx=-1
    for i in range(0, count):
        accountInfo=accountInfos[i]
        nextInstanceFound=False
        for n in range(0, count):
            #myPrint("nextEosIdx: %d, n: %d" % (nextEosIdx, n))
            nextEosIdx=(nextEosIdx + 1)%count
            if eosInstanceInfos[nextEosIdx].alive:
                #myPrint("nextEosIdx: %d" % (nextEosIdx))
                nextInstanceFound=True
                break

        if nextInstanceFound is False:
            myPrint("ERROR: No active nodes found.")
            return False
            
        #myPrint("nextEosIdx: %d, count: %d" % (nextEosIdx, count))
        eosInstanceInfo=eosInstanceInfos[nextEosIdx]
        DEBUG and myPrint("Wait for trasaction id %s on node port %d" % (transId, eosInstanceInfo.port))
        if waitForTransIdOnNode(transId, eosInstanceInfo) is False:
            myPrint("ERROR: Selected node never received transaction id %s" % (transId))
            return False
        
        transferAmount -= amount
        fromm=accountInfo.name
        to=accountInfos[i+1].name if i < (count-1) else adminAccount
        myPrint("Transfer %d units from account %s to %s on eos server port %d." %
              (transferAmount, fromm, to, eosInstanceInfo.port))

        transId=transferFunds(fromm, to, transferAmount, eosInstanceInfo)
        if transId is None:
            return False
        DEBUG and myPrint("Funds transfered on block num %s." % (transId))

        newBalance = accountInfo.balance - transferAmount
        accountInfos[i] = accountInfo._replace(balance=newBalance);
        if i < (count-1):
            newBalance = accountInfos[i+1].balance + transferAmount
            accountInfos[i+1] = accountInfos[i+1]._replace(balance=newBalance);

    # As an extra step wait for last transaction on the root node
    eosInstanceInfo=eosInstanceInfos[0]
    DEBUG and myPrint("Wait for trasaction id %s on node port %d" % (transId, eosInstanceInfo.port))
    if waitForTransIdOnNode(transId, eosInstanceInfo) is False:
        myPrint("ERROR: Selected node never received transaction id %s" % (transId))
        return False
    
    return True

def validateSpreadFunds(adminAccount, accountInfos, eosInstanceInfos, expectedTotal):
    for eosInstanceInfo in eosInstanceInfos:
        if eosInstanceInfo.alive:
            DEBUG and myPrint("Validate funds on eosd server port %d." % (eosInstanceInfo.port))
            if validateSpreadFundsOnNode(adminAccount, accountInfos, eosInstanceInfo, expectedTotal) is False:
                myPrint("ERROR: Failed to validate funds on eos node port: %d" % (eosInstanceInfo.port))
                return False

    return True

def validateSpreadFundsOnNode(adminAccount, accountInfos, eosInstanceInfo, expectedTotal):
    actualTotal=getAccountBalance(adminAccount, eosInstanceInfo.port)
    for accountInfo in accountInfos:
        fund = getAccountBalance(accountInfo.name, eosInstanceInfo.port)
        if fund != accountInfo.balance:
            myPrint("ERROR: validateSpreadFunds> Expected: %d, actual: %d for account %s" % (accountInfo.balance, fund, accountInfo.name))
            return False
        actualTotal += fund

    if actualTotal != expectedTotal:
        myPrint("ERROR: validateSpreadFunds> Expected total: %d , actual: %d" % (expectedTotal, actualTotal))
        return False
    
    return True

def getSystemBalance(adminAccount, accountInfos, port=START_PORT):
    balance=getAccountBalance(adminAccount, port)
    for accountInfo in accountInfos:
        balance += getAccountBalance(accountInfo.name, port)
    return balance

def spreadFundsAndValidate(adminAccount, accountInfos, eosInstanceInfos, amount=1):
    DEBUG and myPrint("Get system balance.")
    initialFunds=getSystemBalance(adminAccount, accountInfos, eosInstanceInfos[0].port)
    DEBUG and myPrint("Initial system balance: %d" % (initialFunds))

    if False == spreadFunds(adminAccount, accountInfos, eosInstanceInfos, amount):
        errorExit("Failed to spread funds across nodes.")

    myPrint("Funds spread across all accounts")

    myPrint("Validate funds.")
    if False == validateSpreadFunds(adminAccount, accountInfos, eosInstanceInfos, initialFunds):
        errorExit("Failed to validate funds transfer across nodes.")

def getAccountBalance(name, port=START_PORT):
    try:
        cmd="programs/eosc/eosc --port %d get account %s" % (port, name)
        DEBUG and myPrint("cmd: %s" % (cmd))
        ret=subprocess.check_output(cmd.split()).decode("utf-8")
        #myPrint ("getAccountBalance>", ret)
        p=re.compile('\n\s+\"eos_balance\"\:\s+\"(\d+.\d+)\s+EOS\",\n', re.MULTILINE)
        m=p.search(ret)
        if m is None:
            msg="Failed to parse account balance."
            myPrint("ERROR: "+ msg)
            raise Exception(msg)

        s=m.group(1)
        balance=float(s)
        balance *= 10000
        return int(balance)

    except Exception as e:
        myPrint("ERROR: Exception during account balance retrieval.", e)
        raise

    
def createAccount(accountInfo, eosInstanceInfo):
    cmd="programs/eosc/eosc create account %s %s %s %s" % (ADMIN_ACCOUNT, accountInfo.name, accountInfo.ownerPublic, accountInfo.activePublic)
    DEBUG and myPrint("cmd: %s" % (cmd))
    if 0 != subprocess.call(cmd.split(), stdout=FNULL):
        myPrint("ERROR: Failed to create inita account.")
        return False

    try :
        cmd="programs/eosc/eosc get account %s" % (accountInfo.name)
        DEBUG and myPrint("cmd: %s" % (cmd))
        ret=subprocess.check_output(cmd.split()).decode("utf-8")
        #myPrint ("ret:", ret)
        p=re.compile('\n\s+\"staked_balance\"\:\s+\"\d+.\d+\s+EOS\",\n', re.MULTILINE)
        m=p.search(ret)
        if m is None:
            myPrint("ERROR: Failed to validate account creation.", accountInfo.name)
            return False
    except Exception as e:
        myPrint("ERROR: Exception during account creation validation:", e)
        return False
    
    return True

# Populates list of EosInstanceInfo objects, matched to actuall running instances
def getEosInstanceInfos(totalNodes):
    eosInstanceInfos=[]

    try:
        cmd="pgrep -a eosd"
        DEBUG and myPrint("cmd: %s" % (cmd))
        psOut=subprocess.check_output(cmd.split()).decode("utf-8")
        #myPrint("psOut: <%s>" % psOut)

        for i in range(0, totalNodes):
            pattern="[\n]?(\d+) (.* --data-dir tn_data_%02d)\n" % (i)
            m=re.search(pattern, psOut, re.MULTILINE)
            if m is None:
                myPrint("ERROR: Failed to find eosd pid. Pattern %s" % (pattern))
                break
            instance=EosInstanceInfo(START_PORT + i, int(m.group(1)), m.group(2), True)
            DEBUG and myPrint("EosInstanceInfo:", instance)
            eosInstanceInfos.append(instance)
            
    except Exception as ex:
        myPrint("ERROR: Exception during EosInstanceInfos creation:", ex)
        
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
def killSomeEosInstances(eosInstanceInfos, percent, killSignal=signal.SIGTERM):
    killCount=int((percent/100.0)*len(eosInstanceInfos))
    DEBUG and myPrint("Kill %d Eosd instances with signal %s." % (killCount, killSignal))

    killedCount=0
    for eosInstanceInfo in reversed(eosInstanceInfos):
        try:
            os.kill(eosInstanceInfo.pid, killSignal)
        except Exception as ex:
            myPrint("ERROR: Failed to kill process pid %d." % (eosInstanceInfo.pid), ex)
            return False
        killedCount += 1
        if killedCount >= killCount:
            break

    time.sleep(1) # Give processes time to stand down
    return updateEosInstanceInfos(eosInstanceInfos)

def relaunchEosInstances(eosInstanceInfos):

    chainArg=chainSyncStrategy.arg

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
                DEBUG and myPrint("cmd: %s" % (cmd))
                popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
                eosInstanceInfos[i]=eosInstanceInfo._replace(pid=popen.pid)

    updateEosInstanceInfos(eosInstanceInfos)
    return True

def launcheCluster(pnodes, total_nodes, topo, delay):
    cmd="programs/launcher/launcher --eosd \"--plugin eosio::wallet_api_plugin\" -p {0} -n {1} -s {2} -d {3}".format(pnodes, total_nodes, topo, delay)
    DEBUG and myPrint("cmd: %s" % (cmd))
    if 0 != subprocess.call(["programs/launcher/launcher", "--eosd", "--plugin eosio::wallet_api_plugin", "-p", str(pnodes), "-n", str(total_nodes), "-d",
                             str(delay), "-s", topo]):
        myPrint("ERROR: Launcher failed to launch.")
        return None

    eosInstanceInfos=getEosInstanceInfos(total_nodes)
    return eosInstanceInfos

def createAccounts(accountInfos, eosInstanceInfo):
    for accountInfo in accountInfos:
        if createAccount(accountInfo, eosInstanceInfo) is False:
            myPrint("ERROR: Failed to create account %s." % (accountInfo.name))
            return False
        DEBUG and myPrint("Account %s created." % (accountInfo.name))

    return True

initializeChainStrategies()

parser = argparse.ArgumentParser()
parser.add_argument("-p", type=int, help="producing nodes count", default=2)
parser.add_argument("-d", type=int, help="delay between nodes startup", default=1)
parser.add_argument("-s", type=str, help="topology", default="mesh")
parser.add_argument("-c", type=str, help="chain strategy[replay|resync|none]", default="none")
parser.add_argument("-v", help="verbose", action='store_true')

args = parser.parse_args()
pnodes=args.p
topo=args.s
delay=args.d
chainSyncStrategyStr=args.c
DEBUG=args.v
total_nodes = pnodes

myPrint ("producing nodes: %d, topology: %s, delay between nodes launch(seconds): %d, chain sync strategy: %s" % (pnodes, topo, delay, chainSyncStrategyStr))

chainSyncStrategy=chainSyncStrategies.get(chainSyncStrategyStr, chainSyncStrategies["none"])

killall({})
cleanup()
random.seed(1) # Use a fixed seed for repeatability.
eosInstanceInfos=None
try:
    print("Stand up cluster")
    eosInstanceInfos=launcheCluster(pnodes, total_nodes, topo, delay)
    if None == eosInstanceInfos:
        errorExit("Failed to stand up eos cluster.")

    if total_nodes != len(eosInstanceInfos):
        errorExit("ERROR: Unable to validate eosd instances, expected: %d, actual: %d" %
                  (total_nodes, len(eosInstanceInfos)))

    myPrint ("Wait for Cluster stabilization")
    # wait for cluster to start producing blocks
    if not waitOnClusterBlockNumSync(eosInstanceInfos, 3):
        errorExit("Cluster never stabilized")

    myPrint ("Create account keys.")
    accountsCount=total_nodes
    accountInfos = createAccountInfos(accountsCount)
    if len(accountInfos) != accountsCount:
        errorExit("Account keys creation failed.")

    myPrint ("Get account %s balance." % (ADMIN_ACCOUNT))
    SYSTEM_BALANCE=getAccountBalance(ADMIN_ACCOUNT, eosInstanceInfos[0].port)

    myPrint ("Create wallet.")
    if not createWallet(INITA_PRV_KEY, accountInfos):
        errorExit("Wallet creation failed.")

    myPrint("Create accounts.")
    if not createAccounts(accountInfos, eosInstanceInfos[0]):
        errorExit("Accounts creation failed.")

    myPrint("Wait on next block after accounts creation")
    waitForNextBlock(eosInstanceInfos[0])
    
    myPrint("Spread funds and validate")
    spreadFundsAndValidate(ADMIN_ACCOUNT, accountInfos, eosInstanceInfos, 10)

    killSignal=signal.SIGTERM
    if chainSyncStrategy.id is 0:
        killSignal=signal.SIGKILL
    
    myPrint("Kill %d%% cluster node instances." % (KILL_PERCENT))
    if killSomeEosInstances(eosInstanceInfos, KILL_PERCENT, killSignal) is False:
        errorExit("Failed to kill Eos instances")
    myPrint("Eosd instances killed.")

    myPrint("Spread funds and validate")
    spreadFundsAndValidate(ADMIN_ACCOUNT, accountInfos, eosInstanceInfos, 10)

    myPrint ("Relaunch dead cluster nodes instances.")
    if relaunchEosInstances(eosInstanceInfos) is False:
        errorExit("Failed to relaunch Eos instances")
    myPrint("Eosd instances relaunched.")

    myPrint ("Resyncing cluster nodes.")
    if not waitOnClusterSync(eosInstanceInfos):
        errorExit("Cluster never synchronized")
    myPrint ("Cluster synched")

    myPrint("Spread funds and validate")
    spreadFundsAndValidate(ADMIN_ACCOUNT, accountInfos, eosInstanceInfos, 10)
    
finally:
    myPrint("Shut down the cluster and cleanup.")
    killall(eosInstanceInfos)
    cleanup()
    pass
    
exit(0)
