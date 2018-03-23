import copy
import decimal
import subprocess
import time
import glob
import shutil
import time
import os
import platform
from collections import namedtuple
import re
import string
import signal
import time
import datetime
import inspect
import sys
import random
import io
import json

###########################################################################################
class Utils:
    Debug=False
    FNull = open(os.devnull, 'w')

    EosClientPath="programs/cleos/cleos"

    EosWalletName="eosiowd"
    EosWalletPath="programs/eosiowd/"+ EosWalletName

    EosServerName="nodeos"
    EosServerPath="programs/nodeos/"+ EosServerName

    EosLauncherPath="programs/eosio-launcher/eosio-launcher"
    MongoPath="mongo"

    @staticmethod
    def Print(*args, **kwargs):
        stackDepth=len(inspect.stack())-2
        str=' '*stackDepth
        sys.stdout.write(str)
        print(*args, **kwargs)

    SyncStrategy=namedtuple("ChainSyncStrategy", "name id arg")

    SyncNoneTag="none"
    SyncReplayTag="replay"
    SyncResyncTag="resync"

    SigKillTag="kill"
    SigTermTag="term"

    systemWaitTimeout=90

    # mongoSyncTime: nodeos mongodb plugin seems to sync with a 10-15 seconds delay. This will inject
    #  a wait period before the 2nd DB check (if first check fails)
    mongoSyncTime=25
    amINoon=True

    # Configure for the NOON branch
    @staticmethod
    def iAmNotNoon():
        Utils.amINoon=False

        Utils.EosClientPath="programs/eosc/eosc"

        Utils.EosWalletName="eos-walletd"
        Utils.EosWalletPath="programs/eos-walletd/"+ Utils.EosWalletName

        Utils.EosServerName="eosd"
        Utils.EosServerPath="programs/eosd/"+ Utils.EosServerName

    @staticmethod
    def setMongoSyncTime(syncTime):
        Utils.mongoSyncTime=syncTime

    @staticmethod
    def setSystemWaitTimeout(timeout):
        Utils.systemWaitTimeout=timeout

    @staticmethod
    def getChainStrategies():
        chainSyncStrategies={}

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncNoneTag, 0, "")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncReplayTag, 1, "--replay-blockchain")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncResyncTag, 2, "--resync-blockchain")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        return chainSyncStrategies

    @staticmethod
    def checkOutput(cmd):
        assert(isinstance(cmd, list))
        retStr=subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode("utf-8")
        return retStr

    @staticmethod
    def errorExit(msg="", raw=False, errorCode=1):
        Utils.Print("ERROR:" if not raw else "", msg)
        exit(errorCode)

###########################################################################################
class Table(object):
    def __init__(self, name):
        self.name=name
        self.keys=[]
        self.data=[]

###########################################################################################
class Transaction(object):
    def __init__(self, transId):
        self.transId=transId
        self.tType=None
        self.amount=0

###########################################################################################
class Account(object):
    def __init__(self, name):
        self.name=name
        self.balance=0

        self.ownerPrivateKey=None
        self.ownerPublicKey=None
        self.activePrivateKey=None
        self.activePublicKey=None


    def __str__(self):
        return "Name; %s" % (self.name)

###########################################################################################
class Node(object):

    def __init__(self, host, port, pid=None, cmd=None, alive=None, enableMongo=False, mongoHost="localhost", mongoPort=27017, mongoDb="EOStest"):
        self.host=host
        self.port=port
        self.pid=pid
        self.cmd=cmd
        self.alive=alive
        self.enableMongo=enableMongo
        self.mongoSyncTime=None if Utils.mongoSyncTime < 1 else Utils.mongoSyncTime
        self.mongoHost=mongoHost
        self.mongoPort=mongoPort
        self.mongoDb=mongoDb
        self.endpointArgs="--host %s --port %d" % (self.host, self.port)
        self.mongoEndpointArgs=""
        if self.enableMongo:
            self.mongoEndpointArgs += "--host %s --port %d %s" % (mongoHost, mongoPort, mongoDb)

    def __str__(self):
        #return "Host: %s, Port:%d, Pid:%s, Alive:%s, Cmd:\"%s\"" % (self.host, self.port, self.pid, self.alive, self.cmd)
        return "Host: %s, Port:%d" % (self.host, self.port)

    @staticmethod
    def runCmdReturnJson(cmd, trace=False):
        retStr=Utils.checkOutput(cmd.split())
        jStr=Node.filterJsonObject(retStr)
        trace and Utils.Print ("RAW > %s"% retStr)
        trace and Utils.Print ("JSON> %s"% jStr)
        try:
            jsonData=json.loads(jStr)
        except json.decoder.JSONDecodeError as ex:
            Utils.Print ("RAW > %s"% retStr)
            Utils.Print ("JSON> %s"% jStr)

        return jsonData

    @staticmethod
    def __runCmdArrReturnJson(cmdArr, trace=False):
        retStr=Utils.checkOutput(cmdArr)
        jStr=Node.filterJsonObject(retStr)
        trace and Utils.Print ("RAW > %s"% retStr)
        trace and Utils.Print ("JSON> %s"% jStr)
        jsonData=json.loads(jStr)
        return jsonData

    @staticmethod
    def runCmdReturnStr(cmd, trace=False):
        retStr=Node.__checkOutput(cmd.split())
        trace and Utils.Print ("RAW > %s"% retStr)
        return retStr

    @staticmethod
    def filterJsonObject(data):
        firstIdx=data.find('{')
        lastIdx=data.rfind('}')
        retStr=data[firstIdx:lastIdx+1]
        return retStr

    @staticmethod
    def __checkOutput(cmd):
        retStr=subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode("utf-8")
        #retStr=subprocess.check_output(cmd).decode("utf-8")
        return retStr


    # Passes input to stdin, executes cmd. Returns tuple with return code(int),
    #  stdout(byte stream) and stderr(byte stream).
    @staticmethod
    def stdinAndCheckOutput(cmd, subcommand):
        outs=None
        errs=None
        try:
            popen=subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            outs,errs=popen.communicate(input=subcommand.encode("utf-8"))
            ret=popen.wait()
        except subprocess.CalledProcessError as ex:
            msg=ex.output
            return (ex.returncode, msg, None)

        return (0, outs, errs)

    @staticmethod
    def normalizeJsonObject(extJStr):
        tmpStr=extJStr
        tmpStr=re.sub(r'ObjectId\("(\w+)"\)', r'"ObjectId-\1"', tmpStr)
        tmpStr=re.sub(r'ISODate\("([\w|\-|\:|\.]+)"\)', r'"ISODate-\1"', tmpStr)
        return tmpStr

    @staticmethod
    def runMongoCmdReturnJson(cmdArr, subcommand, trace=False):
        retId,outs,errs=Node.stdinAndCheckOutput(cmdArr, subcommand)
        if retId is not 0:
            return None
        outStr=Node.byteArrToStr(outs)
        if not outStr:
            return None
        extJStr=Node.filterJsonObject(outStr)
        if not extJStr:
            return None
        jStr=Node.normalizeJsonObject(extJStr)
        if not jStr:
            return None
        trace and Utils.Print ("RAW > %s"% outStr)
        #trace and Utils.Print ("JSON> %s"% jStr)
        jsonData=json.loads(jStr)
        return jsonData

    @staticmethod
    def getTransId(trans):
        #Utils.Print("%s" % trans)
        transId=trans["transaction_id"]
        return transId

    @staticmethod
    def byteArrToStr(arr):
        return arr.decode("utf-8")

    def setWalletEndpointArgs(self, args):
        self.endpointArgs="--host %s --port %d %s" % (self.host, self.port, args)

    def getBlock(self, blockNum, retry=True, silentErrors=False):
        if not self.enableMongo:
            cmd="%s %s get block %s" % (Utils.EosClientPath, self.endpointArgs, blockNum)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            try:
                trans=Node.runCmdReturnJson(cmd)
                return trans
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    msg=ex.output.decode("utf-8")
                    Utils.Print("ERROR: Exception during get block. %s" % (msg))
                return None
        else:
            for i in range(2):
                cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
                subcommand='db.Blocks.findOne( { "block_num": %s } )' % (blockNum)
                Utils.Debug and Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
                try:
                    trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
                    if trans is not None:
                        return trans
                except subprocess.CalledProcessError as ex:
                    if not silentErrors:
                        msg=ex.output.decode("utf-8")
                        Utils.Print("ERROR: Exception during get db node get block. %s" % (msg))
                    return None
                if not retry:
                    break
                if self.mongoSyncTime is not None:
                    Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                    time.sleep(self.mongoSyncTime)

        return None

    def getBlockById(self, blockId, retry=True, silentErrors=False):
        for i in range(2):
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand='db.Blocks.findOne( { "block_id": "%s" } )' % (blockId)
            Utils.Debug and Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
            try:
                trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
                if trans is not None:
                    return trans
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    msg=ex.output.decode("utf-8")
                    Utils.Print("ERROR: Exception during db get block by id. %s" % (msg))
                return None
            if not retry:
                break
            if self.mongoSyncTime is not None:
                Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                time.sleep(self.mongoSyncTime)

        return None


    def doesNodeHaveBlockNum(self, blockNum):
        assert isinstance(blockNum, int)

        info=self.getInfo(silentErrors=True)
        last_irreversible_block_num=int(info["last_irreversible_block_num"])
        if blockNum > last_irreversible_block_num:
            return False
        else:
            return True

    def getTransaction(self, transId, retry=True, silentErrors=False):
        if not self.enableMongo:
            cmd="%s %s get transaction %s" % (Utils.EosClientPath, self.endpointArgs, transId)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            try:
                trans=Node.runCmdReturnJson(cmd)
                return trans
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    msg=ex.output.decode("utf-8")
                    Utils.Print("ERROR: Exception during account by transaction retrieval. %s" % (msg))
                return None
        else:
            for i in range(2):
                cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
                subcommand='db.Transactions.findOne( { $and : [ { "transaction_id": "%s" }, {"pending":false} ] } )' % (transId)
                Utils.Debug and Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
                try:
                    trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
                    return trans
                except subprocess.CalledProcessError as ex:
                    if not silentErrors:
                        msg=ex.output.decode("utf-8")
                        Utils.Print("ERROR: Exception during get db node get trans. %s" % (msg))
                    return None
                if not retry:
                    break
                if self.mongoSyncTime is not None:
                    Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                    time.sleep(self.mongoSyncTime)

        return None

    def getTransByBlockId(self, blockId, retry=True, silentErrors=False):
        for i in range(2):
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand='db.Transactions.find( { "block_id": "%s" } )' % (blockId)
            Utils.Debug and Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
            try:
                trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand, True)
                if trans is not None:
                    return trans
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    msg=ex.output.decode("utf-8")
                    Utils.Print("ERROR: Exception during db get trans by blockId. %s" % (msg))
                return None
            if not retry:
                break
            if self.mongoSyncTime is not None:
                Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                time.sleep(self.mongoSyncTime)

        return None



    def getActionFromDb(self, transId, retry=True, silentErrors=False):
        for i in range(2):
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand='db.Actions.findOne( { "transaction_id": "%s" } )' % (transId)
            Utils.Debug and Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
            try:
                trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
                if trans is not None:
                    return trans
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    msg=ex.output.decode("utf-8")
                    Utils.Print("ERROR: Exception during get db node get message. %s" % (msg))
                return None
            if not retry:
                break
            if self.mongoSyncTime is not None:
                Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                time.sleep(self.mongoSyncTime)

        return None

    def getMessageFromDb(self, transId, retry=True, silentErrors=False):
        for i in range(2):
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand='db.Messages.findOne( { "transaction_id": "%s" } )' % (transId)
            Utils.Debug and Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
            try:
                trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
                if trans is not None:
                    return trans
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    msg=ex.output.decode("utf-8")
                    Utils.Print("ERROR: Exception during get db node get message. %s" % (msg))
                return None
            if not retry:
                break
            if self.mongoSyncTime is not None:
                Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                time.sleep(self.mongoSyncTime)

        return None

    def doesNodeHaveTransId(self, transId):
        trans=self.getTransaction(transId, silentErrors=True)
        if trans is None:
            return False

        blockNum=None
        if not self.enableMongo:
            blockNum=int(trans["transaction"]["data"]["ref_block_num"])
        else:
            blockNum=int(trans["ref_block_num"])

        blockNum += 1
        return self.doesNodeHaveBlockNum(blockNum)

    def createInitAccounts(self, producers):
        """Initializes accounts. Requires eosio account. Creates init accounts and funds them."""
        eosio = Account("eosio")
        # eosio = self.eosioAccount

        # publish system contract
        wastFile="contracts/eosio.system/eosio.system.wast"
        abiFile="contracts/eosio.system/eosio.system.abi"
        Utils.Print("Publish eosio.system contract")
        trans=self.publishContract(eosio.name, wastFile, abiFile, waitForTransBlock=True)
        if trans is None:
           Utils.errorExit("Failed to publish eosio.system.")

        Utils.Print("push issue action to eosio contract")
        contract=eosio.name
        action="issue"
        data="{\"to\":\"eosio\",\"quantity\":\"1000000000.0000 EOS\"}"
        opts="--permission eosio@active"
        trans=self.pushMessage(contract, action, data, opts)
        transId=Node.getTransId(trans[1])
        self.waitForTransIdOnNode(transId)

        expectedAmount=10000000000000
        Utils.Print("Verify eosio issue, Expected: %d" % (expectedAmount))
        actualAmount=self.getAccountBalance(eosio.name)
        if expectedAmount != actualAmount:
            Utils.errorExit("Issue verification failed. Excepted %d, actual: %d" % (expectedAmount, actualAmount))

        trans=None
        for name, keys in producers.items():
            if name == eosio.name:
                continue
            initx = Account(name)
            initx.ownerPrivateKey=keys[1]
            initx.ownerPublicKey=keys[0]
            initx.activePrivateKey=keys[1]
            initx.activePublicKey=keys[0]
            trans=self.createAccount(initx, eosio, 0)
            if trans is None:
                Utils.errorExit("Failed to create account %s" % (name))

            trans = self.transferFunds(eosio, initx, 10000000000, "init transfer")
            if trans is None:
                Utils.errorExit("Failed to transfer funds from %s to %s." % (eosio.name, name))

        transId=Node.getTransId(trans)
        self.waitForTransIdOnNode(transId)

        return None

    # Create account and return creation transactions. Return transaction json object
    # waitForTransBlock: wait on creation transaction id to appear in a block
    def createAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False):
        cmd=None
        if Utils.amINoon:
            cmd="%s %s create account %s %s %s %s" % (
                Utils.EosClientPath, self.endpointArgs, creatorAccount.name, account.name,
                account.ownerPublicKey, account.activePublicKey)
        else:
            cmd="%s %s create account %s %s %s %s" % (Utils.EosClientPath, self.endpointArgs,
                                                      creatorAccount.name, account.name,
                                                      account.ownerPublicKey, account.activePublicKey)

        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        trans=None
        try:
            trans=Node.runCmdReturnJson(cmd)
            transId=Node.getTransId(trans)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during account creation. %s" % (msg))
            return None

        if waitForTransBlock and not self.waitForTransIdOnNode(transId):
            return None

        if stakedDeposit > 0:
            trans = self.transferFunds(creatorAccount, account, stakedDeposit, "init")
            transId=Node.getTransId(trans)
            if waitForTransBlock and not self.waitForTransIdOnNode(transId):
                return None
        return trans

    def getEosAccount(self, name):
        cmd="%s %s get account %s" % (Utils.EosClientPath, self.endpointArgs, name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get account. %s" % (msg))
            return None

    def getEosAccountFromDb(self, name):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand='db.Accounts.findOne({"name" : "%s"})' % (name)
        Utils.Debug and Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
        try:
            trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get account from db. %s" % (msg))
            return None


    def getEosCurrencyBalance(self, name):
        cmd="%s %s get currency balance eosio %s EOS" % (Utils.EosClientPath, self.endpointArgs, name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnStr(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get EOS balance. %s" % (msg))
            return None

    def getCurrencyBalance(self, contract, account, symbol):
        cmd="%s %s get currency balance %s %s %s" % (Utils.EosClientPath, self.endpointArgs, contract, account, symbol)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnStr(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get currency balance. %s" % (msg))
            return None

    def getCurrencyStats(self, contract, symbol=""):
        cmd="%s %s get currency stats %s %s" % (Utils.EosClientPath, self.endpointArgs, contract, symbol)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get currency stats. %s" % (msg))
            return None

    # Verifies account. Returns "get account" json return object
    def verifyAccount(self, account):
        if not self.enableMongo:
            ret=self.getEosAccount(account.name)
            if ret is not None:
                account_name=ret["account_name"]
                if account_name is None:
                    Utils.Print("ERROR: Failed to verify account creation.", account.name)
                    return None
                return ret
        else:
            for i in range(2):
                ret=self.getEosAccountFromDb(account.name)
                if ret is not None:
                    account_name=ret["name"]
                    if account_name is None:
                        Utils.Print("ERROR: Failed to verify account creation.", account.name)
                        return None
                    return ret
                if self.mongoSyncTime is not None:
                    Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                    time.sleep(self.mongoSyncTime)

        return None

    def waitForBlockNumOnNode(self, blockNum, timeout=None):
        if timeout is None:
            timeout=Utils.systemWaitTimeout
        startTime=time.time()
        remainingTime=timeout
        Utils.Debug and Utils.Print("cmd: remaining time %d seconds" % (remainingTime))
        while time.time()-startTime < timeout:
            if self.doesNodeHaveBlockNum(blockNum):
                return True
            sleepTime=3 if remainingTime > 3 else (3 - remainingTime)
            remainingTime -= sleepTime
            Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (sleepTime))
            time.sleep(sleepTime)

        return False

    def waitForTransIdOnNode(self, transId, timeout=None):
        if timeout is None:
            timeout=Utils.systemWaitTimeout
        startTime=time.time()
        remainingTime=timeout
        Utils.Debug and Utils.Print("cmd: remaining time %d seconds" % (remainingTime))
        while time.time()-startTime < timeout:
            if self.doesNodeHaveTransId(transId):
                return True
            sleepTime=3 if remainingTime > 3 else (3 - remainingTime)
            remainingTime -= sleepTime
            Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (sleepTime))
            time.sleep(sleepTime)

        return False

    def waitForNextBlock(self, timeout=None):
        if timeout is None:
            timeout=Utils.systemWaitTimeout
        startTime=time.time()
        remainingTime=timeout
        Utils.Debug and Utils.Print("cmd: remaining time %d seconds" % (remainingTime))
        num=self.getIrreversibleBlockNum()
        Utils.Debug and Utils.Print("Current block number: %s" % (num))

        while time.time()-startTime < timeout:
            nextNum=self.getIrreversibleBlockNum()
            if nextNum > num:
                Utils.Debug and Utils.Print("Next block number: %s" % (nextNum))
                return True

            sleepTime=.5 if remainingTime > .5 else (.5 - remainingTime)
            remainingTime -= sleepTime
            Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (sleepTime))
            time.sleep(sleepTime)

        return False

    # Trasfer funds. Returns "transfer" json return object
    def transferFunds(self, source, destination, amount, memo="memo", force=False):
        cmd="%s %s -v transfer %s %s %d" % (
            Utils.EosClientPath, self.endpointArgs, source.name, destination.name, amount)
        cmdArr=cmd.split()
        cmdArr.append(memo)
        if force:
            cmdArr.append("-f")
        s=" ".join(cmdArr)
        Utils.Debug and Utils.Print("cmd: %s" % (s))
        trans=None
        try:
            trans=Node.__runCmdArrReturnJson(cmdArr)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during funds transfer. %s" % (msg))
            return None

    def validateSpreadFundsOnNode(self, adminAccount, accounts, expectedTotal):
        actualTotal=self.getAccountBalance(adminAccount.name)
        for account in accounts:
            fund = self.getAccountBalance(account.name)
            if fund != account.balance:
                Utils.Print("ERROR: validateSpreadFunds> Expected: %d, actual: %d for account %s" %
                        (account.balance, fund, account.name))
                return False
            actualTotal += fund

        if actualTotal != expectedTotal:
            Utils.Print("ERROR: validateSpreadFunds> Expected total: %d , actual: %d" % (
                expectedTotal, actualTotal))
            return False

        return True

    def getSystemBalance(self, adminAccount, accounts):
        balance=self.getAccountBalance(adminAccount.name)
        for account in accounts:
            balance += self.getAccountBalance(account.name)
        return balance

    # Gets accounts mapped to key. Returns json object
    def getAccountsByKey(self, key):
        cmd="%s %s get accounts %s" % (Utils.EosClientPath, self.endpointArgs, key)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during accounts by key retrieval. %s" % (msg))
            return None

    # Gets accounts mapped to key. Returns array
    def getAccountsArrByKey(self, key):
        trans=self.getAccountsByKey(key)
        accounts=trans["account_names"]
        return accounts

    def getServants(self, name):
        cmd="%s %s get servants %s" % (Utils.EosClientPath, self.endpointArgs, name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during servants retrieval. %s" % (msg))
            return None

    def getServantsArr(self, name):
        trans=self.getServants(name)
        servants=trans["controlled_accounts"]
        return servants

    def getAccountBalance(self, name):
        if not self.enableMongo:
            amount=self.getEosCurrencyBalance(name)
            Utils.Debug and Utils.Print("getEosCurrencyBalance", name, amount)
            balanceStr=amount.split()[0]
            balance=int(decimal.Decimal(balanceStr[1:])*10000)
            return balance
        else:
            if self.mongoSyncTime is not None:
                Utils.Debug and Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                time.sleep(self.mongoSyncTime)

            account=self.getEosAccountFromDb(name)
            if account is not None:
                field=account["eos_balance"]
                balanceStr=field.split()[0]
                balance=int(float(balanceStr)*10000)
                return balance

        return None

    # transactions lookup by id. Returns json object
    def getTransactionsByAccount(self, name):
        cmd="%s %s get transactions %s" % (Utils.EosClientPath, self.endpointArgs, name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during transactions by account retrieval. %s" % (msg))
            return None

    # transactions lookup by id. Returns list of transaction ids
    def getTransactionsArrByAccount(self, name):
        trans=self.getTransactionsByAccount(name)
        transactions=trans["transactions"]
        transArr=[]
        for transaction in transactions:
            id=transaction["transaction_id"]
            transArr.append(id)
        return transArr

    def getAccountCodeHash(self, account):
        cmd="%s %s get code %s" % (Utils.EosClientPath, self.endpointArgs, account)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            retStr=Utils.checkOutput(cmd.split())
            #Utils.Print ("get code> %s"% retStr)
            p=re.compile('code\shash: (\w+)\n', re.MULTILINE)
            m=p.search(retStr)
            if m is None:
                msg="Failed to parse code hash."
                Utils.Print("ERROR: "+ msg)
                return None

            return m.group(1)
            trans=Node.runCmdReturnJson(cmd, True)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during code hash retrieval. %s" % (msg))
            return None

    # publish contract and return transaction as json object
    def publishContract(self, account, wastFile, abiFile, waitForTransBlock=False, shouldFail=False):
        cmd="%s %s -v set contract %s %s %s" % (Utils.EosClientPath, self.endpointArgs, account, wastFile, abiFile)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        trans=None
        try:
            trans=Node.runCmdReturnJson(cmd)
        except subprocess.CalledProcessError as ex:
            if not shouldFail:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during code hash retrieval. %s" % (msg))
                return None
            else:
                retMap={}
                retMap["returncode"]=ex.returncode
                retMap["cmd"]=ex.cmd
                retMap["output"]=ex.output
                # commented below as they are available only in Python3.5 and above
                # retMap["stdout"]=ex.stdout
                # retMap["stderr"]=ex.stderr
                return retMap

        if shouldFail:
            Utils.Print("ERROR: The publish contract did not fail as expected.")
            return None

        transId=Node.getTransId(trans)
        if waitForTransBlock and not self.waitForTransIdOnNode(transId):
            return None
        return trans

    # create producer and return transaction as json object
    def createProducer(self, account, ownerPublicKey, waitForTransBlock=False):
        cmd="%s %s create producer %s %s" % (Utils.EosClientPath, self.endpointArgs, account, ownerPublicKey)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        trans=None
        try:
            trans=Node.runCmdReturnJson(cmd)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during producer creation. %s" % (msg))
            return None

        transId=Node.getTransId(trans)
        if waitForTransBlock and not self.waitForTransIdOnNode(transId):
            return None
        return trans

    def getTable(self, account, contract, table):
        cmd="%s %s get table %s %s %s" % (Utils.EosClientPath, self.endpointArgs, account, contract, table)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during table retrieval. %s" % (msg))
            return None

    def getTableRows(self, account, contract, table):
        jsonData=self.getTable(account, contract, table)
        if jsonData is None:
            return None
        rows=jsonData["rows"]
        return rows

    def getTableRow(self, account, contract, table, idx):
        if idx < 0:
            Utils.Print("ERROR: Table index cannot be negative. idx: %d" % (idx))
            return None
        rows=self.getTableRows(account, contract, table)
        if rows is None or idx >= len(rows):
            Utils.Print("ERROR: Retrieved table does not contain row %d" % idx)
            return None
        row=rows[idx]
        return row

    def getTableColumns(self, account, contract, table):
        row=self.getTableRow(account, contract, table, 0)
        keys=list(row.keys())
        return keys

    # returns tuple with transaction and
    def pushMessage(self, contract, action, data, opts, silentErrors=False):
        cmd=None
        if Utils.amINoon:
            cmd="%s %s push action %s %s" % (Utils.EosClientPath, self.endpointArgs, contract, action)
        else:
            cmd="%s %s push message %s %s" % (Utils.EosClientPath, self.endpointArgs, contract, action)
        cmdArr=cmd.split()
        if data is not None:
            cmdArr.append(data)
        if opts is not None:
            cmdArr += opts.split()
        s=" ".join(cmdArr)
        Utils.Debug and Utils.Print("cmd: %s" % (s))
        try:
            trans=Node.__runCmdArrReturnJson(cmdArr)
            return (True, trans)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            if not silentErrors:
                Utils.Print("ERROR: Exception during push message. %s" % (msg))
            return (False, msg)

    def setPermission(self, account, code, pType, requirement, waitForTransBlock=False):
        cmd="%s %s set action permission %s %s %s %s" % (
            Utils.EosClientPath, self.endpointArgs, account, code, pType, requirement)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        trans=None
        try:
            trans=Node.runCmdReturnJson(cmd)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during set permission. %s" % (msg))
            return None

        transId=Node.getTransId(trans)
        if waitForTransBlock and not self.waitForTransIdOnNode(transId):
            return None
        return trans

    def getInfo(self, silentErrors=False):
        cmd="%s %s get info" % (Utils.EosClientPath, self.endpointArgs)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            if not silentErrors:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during get info. %s" % (msg))
            return None

    def getBlockFromDb(self, idx):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand="db.Blocks.find().sort({\"_id\":%d}).limit(1).pretty()" % (idx)
        Utils.Debug and Utils.Print("cmd: echo \"%s\" | %s" % (subcommand, cmd))
        try:
            trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get db block. %s" % (msg))
            return None

    def checkPulse(self):
        info=self.getInfo(True)
        if info is not None:
            self.alive=True
            return True
        else:
            self.alive=False
            return False

    def getHeadBlockNum(self):
        if not self.enableMongo:
            info=self.getInfo()
            if info is not None:
                headBlockNumTag="head_block_num"
                return info[headBlockNumTag]
        else:
            block=self.getBlockFromDb(-1)
            if block is not None:
                blockNum=block["block_num"]
                return blockNum
        return None

    def getIrreversibleBlockNum(self):
        if not self.enableMongo:
            info=self.getInfo()
            if info is not None:
                return info["last_irreversible_block_num"]
        else:
            block=self.getBlockFromDb(-1)
            if block is not None:
                blockNum=block["block_num"]
                return blockNum
        return None

###########################################################################################

Wallet=namedtuple("Wallet", "name password host port")
class WalletMgr(object):
    __walletLogFile="test_walletd_output.log"
    __walletDataDir="test_wallet_0"

    # walletd [True|False] Will wallet me in a standalone process
    def __init__(self, walletd, nodeosPort=8888, nodeosHost="localhost", port=8899, host="localhost"):
        self.walletd=walletd
        self.nodeosPort=nodeosPort
        self.nodeosHost=nodeosHost
        self.port=port
        self.host=host
        self.wallets={}
        self.__walletPid=None
        self.endpointArgs="--host %s --port %d" % (self.nodeosHost, self.nodeosPort)
        if self.walletd:
            self.endpointArgs += " --wallet-host %s --wallet-port %d" % (self.host, self.port)

    def launch(self):
        if not self.walletd:
            Utils.Print("ERROR: Wallet Manager wasn't configured to launch walletd")
            return False

        cmd="%s --data-dir %s --config-dir %s --http-server-address=%s:%d" % (
            Utils.EosWalletPath, WalletMgr.__walletDataDir, WalletMgr.__walletDataDir, self.host, self.port)
        Utils.Print("cmd: %s" % (cmd))
        with open(WalletMgr.__walletLogFile, 'w') as sout, open(WalletMgr.__walletLogFile, 'w') as serr:
            popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
            self.__walletPid=popen.pid

        # Give walletd time to warm up
        time.sleep(1)
        return True

    def create(self, name):
        wallet=self.wallets.get(name)
        if wallet is not None:
            Utils.Debug and Utils.Print("Wallet \"%s\" already exists. Returning same." % name)
            return wallet
        p = re.compile('\n\"(\w+)\"\n', re.MULTILINE)
        cmd="%s %s wallet create --name %s" % (Utils.EosClientPath, self.endpointArgs, name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        retStr=subprocess.check_output(cmd.split()).decode("utf-8")
        #Utils.Print("create: %s" % (retStr))
        m=p.search(retStr)
        if m is None:
            Utils.Print("ERROR: wallet password parser failure")
            return None
        p=m.group(1)
        wallet=Wallet(name, p, self.host, self.port)
        self.wallets[name] = wallet

        return wallet

    def importKey(self, account, wallet):
        warningMsg="This key is already imported into the wallet"
        cmd="%s %s wallet import --name %s %s" % (
            Utils.EosClientPath, self.endpointArgs, wallet.name, account.ownerPrivateKey)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            retStr=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            if warningMsg in msg:
                Utils.Print("WARNING: This key is already imported into the wallet.")
            else:
                Utils.Print("ERROR: Failed to import account owner key %s. %s" % (account.ownerPrivateKey, msg))
                return False

        if account.activePrivateKey is None:
            Utils.Print("WARNING: Active private key is not defined for account \"%s\"" % (account.name))
        else:
            cmd="%s %s wallet import --name %s %s" % (
                Utils.EosClientPath, self.endpointArgs, wallet.name, account.activePrivateKey)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            try:
                retStr=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
            except subprocess.CalledProcessError as ex:
                msg=ex.output.decode("utf-8")
                if warningMsg in msg:
                    Utils.Print("WARNING: This key is already imported into the wallet.")
                else:
                    Utils.Print("ERROR: Failed to import account active key %s. %s" %
                                (account.activePrivateKey, msg))
                    return False

        return True

    def lockWallet(self, wallet):
        cmd="%s %s wallet lock --name %s" % (Utils.EosClientPath, self.endpointArgs, wallet.name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            Utils.Print("ERROR: Failed to lock wallet %s." % (wallet.name))
            return False

        return True

    def unlockWallet(self, wallet):
        cmd="%s %s wallet unlock --name %s" % (Utils.EosClientPath, self.endpointArgs, wallet.name)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        popen=subprocess.Popen(cmd.split(), stdout=Utils.FNull, stdin=subprocess.PIPE)
        outs, errs = popen.communicate(input=wallet.password.encode("utf-8"))
        if 0 != popen.wait():
            Utils.Print("ERROR: Failed to unlock wallet %s: %s" % (wallet.name, errs.decode("utf-8")))
            return False

        return True

    def lockAllWallets(self):
        cmd="%s %s wallet lock_all" % (Utils.EosClientPath, self.endpointArgs)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            Utils.Print("ERROR: Failed to lock all wallets.")
            return False

        return True

    def getOpenWallets(self):
        wallets=[]

        p = re.compile('\s+\"(\w+)\s\*\",?\n', re.MULTILINE)
        cmd="%s %s wallet list" % (Utils.EosClientPath, self.endpointArgs)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        retStr=subprocess.check_output(cmd.split()).decode("utf-8")
        #Utils.Print("retStr: %s" % (retStr))
        m=p.findall(retStr)
        if m is None:
            Utils.Print("ERROR: wallet list parser failure")
            return None
        wallets=m

        return wallets

    def getKeys(self):
        keys=[]

        p = re.compile('\n\s+\"(\w+)\"\n', re.MULTILINE)
        cmd="%s %s wallet keys" % (Utils.EosClientPath, self.endpointArgs)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        retStr=subprocess.check_output(cmd.split()).decode("utf-8")
        #Utils.Print("retStr: %s" % (retStr))
        m=p.findall(retStr)
        if m is None:
            Utils.Print("ERROR: wallet keys parser failure")
            return None
        keys=m

        return keys


    def dumpErrorDetails(self):
        Utils.Print("=================================================================")
        if self.__walletPid is not None:
            Utils.Print("Contents of %s:" % (WalletMgr.__walletLogFile))
            Utils.Print("=================================================================")
            with open(WalletMgr.__walletLogFile, "r") as f:
                shutil.copyfileobj(f, sys.stdout)

    def killall(self):
        cmd="pkill %s" % (Utils.EosWalletName)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        subprocess.call(cmd.split())

    def cleanup(self):
        dataDir=WalletMgr.__walletDataDir
        if os.path.isdir(dataDir) and os.path.exists(dataDir):
            shutil.rmtree(WalletMgr.__walletDataDir)



###########################################################################################
class Cluster(object):
    __chainSyncStrategies=Utils.getChainStrategies()
    __WalletName="MyWallet"
    __localHost="localhost"
    __lastTrans=None


    # walletd [True|False] Is walletd running. If not load the wallet plugin
    def __init__(self, walletd=False, localCluster=True, host="localhost", port=8888, walletHost="localhost", walletPort=8899, enableMongo=False, mongoHost="localhost", mongoPort=27017, mongoDb="EOStest", initaPrvtKey=None, initbPrvtKey=None, staging=False):
        self.accounts={}
        self.nodes={}
        self.localCluster=localCluster
        self.wallet=None
        self.walletd=walletd
        self.enableMongo=enableMongo
        self.mongoHost=mongoHost
        self.mongoPort=mongoPort
        self.mongoDb=mongoDb
        self.walletMgr=None
        self.host=host
        self.port=port
        self.walletHost=walletHost
        self.walletPort=walletPort
        self.walletEndpointArgs=""
        if self.walletd:
            self.walletEndpointArgs += " --wallet-host %s --wallet-port %d" % (self.walletHost, self.walletPort)
        self.mongoEndpointArgs=""
        self.mongoUri=""
        if self.enableMongo:
            self.mongoUri="mongodb://%s:%d/%s" % (mongoHost, mongoPort, mongoDb)
            self.mongoEndpointArgs += "--host %s --port %d %s" % (mongoHost, mongoPort, mongoDb)
        self.staging=staging
        # init accounts
        self.initaAccount=Account("inita")
        self.initbAccount=Account("initb")
        self.eosioAccount=Account("eosio")
        self.initaAccount.ownerPrivateKey=initaPrvtKey
        self.initbAccount.ownerPrivateKey=initbPrvtKey
        self.producerKeys=None


    def setChainStrategy(self, chainSyncStrategy=Utils.SyncReplayTag):
        self.__chainSyncStrategy=self.__chainSyncStrategies.get(chainSyncStrategy)
        if self.__chainSyncStrategy is None:
            self.__chainSyncStrategy= __chainSyncStrategies.get("none")

    def setWalletMgr(self, walletMgr):
        self.walletMgr=walletMgr

    # launch local nodes and set self.nodes
    def launch(self, pnodes=1, total_nodes=1, topo="mesh", delay=0):
        if not self.localCluster:
            Utils.Print("WARNING: Cluster not local, not launching %s." % (Utils.EosServerName))
            return True

        if len(self.nodes) > 0:
            raise RuntimeError("Cluster already running.")

        cmd="%s -p %s -n %s -s %s -d %s -f" % (
            Utils.EosLauncherPath, pnodes, total_nodes, topo, delay)
        cmdArr=cmd.split()
        if self.staging:
            cmdArr.append("--nogen")
        if not self.walletd or self.enableMongo:
            if Utils.amINoon:
                cmdArr.append("--nodeos")
            else:
                cmdArr.append("--eosd")
            if not self.walletd:
                cmdArr.append("--plugin eosio::wallet_api_plugin")
            if self.enableMongo:
                if Utils.amINoon:
                    cmdArr.append("--plugin eosio::mongo_db_plugin --resync --mongodb-uri %s" % self.mongoUri)
                else:
                    cmdArr.append("--plugin eosio::db_plugin --mongodb-uri %s" % self.mongoUri)
        s=" ".join(cmdArr)
        Utils.Print("cmd: %s" % (s))
        if 0 != subprocess.call(cmdArr):
            Utils.Print("ERROR: Launcher failed to launch.")
            return False

        self.nodes=range(total_nodes) # placeholder for cleanup purposes only
        nodes=self.discoverLocalNodes(total_nodes)

        if total_nodes != len(nodes):
            Utils.Print("ERROR: Unable to validate %s instances, expected: %d, actual: %d" %
                          (Utils.EosServerName, total_nodes, len(nodes)))
            return False

        self.nodes=nodes

        if not self.parseClusterInfo(total_nodes):
            Utils.Print("ERROR: Unable to parse cluster info")
            return False

        return True

    # Initialize the default nodes (at present just the root node)
    def initializeNodes(self):
        node=Node(self.host, self.port, enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
        node.setWalletEndpointArgs(self.walletEndpointArgs)
        Utils.Debug and Utils.Print("Node:", node)

        node.checkPulse()
        self.nodes=[node]
        return True

    # Initialize nodes from the Json nodes string
    def initializeNodesFromJson(self, nodesJsonStr):
        nodesObj= json.loads(nodesJsonStr)
        if nodesObj is None:
            Utils.Print("ERROR: Invalid Json string.")
            return False

        if "keys" in nodesObj:
            keysMap=nodesObj["keys"]

            if "initaPrivateKey" in keysMap:
                initaPrivateKey=keysMap["initaPrivateKey"]
                self.initaAccount.ownerPrivateKey=initaPrivateKey

            if "initbPrivateKey" in keysMap:
                initbPrivateKey=keysMap["initbPrivateKey"]
                self.initbAccount.ownerPrivateKey=initbPrivateKey

        nArr=nodesObj["nodes"]
        nodes=[]
        for n in nArr:
            port=n["port"]
            host=n["host"]
            node=Node(host, port)
            node.setWalletEndpointArgs(self.walletEndpointArgs)
            Utils.Debug and Utils.Print("Node:", node)

            node.checkPulse()
            nodes.append(node)

        self.nodes=nodes
        return True

    # manually set nodes, alternative to explicit launch
    def setNodes(self, nodes):
        self.nodes=nodes

    # If a last transaction exists wait for it on root node, then collect its head block number.
    #  Wait on this block number on each cluster node
    def waitOnClusterSync(self, timeout=None):
        if timeout is None:
            timeout=Utils.systemWaitTimeout
        startTime=time.time()
        Utils.Debug and Utils.Print("cmd: remaining time %d seconds" % (timeout))
        if self.nodes[0].alive is False:
            Utils.Print("ERROR: Root node is down.")
            return False;

        if self.__lastTrans is not None:
            if self.nodes[0].waitForTransIdOnNode(self.__lastTrans) is False:
                Utils.Print("ERROR: Failed to wait for last known transaction(%s) on root node." %
                            (self.__lastTrans))
                return False;

        targetHeadBlockNum=self.nodes[0].getHeadBlockNum() #get root nodes head block num
        Utils.Debug and Utils.Print("Head block number on root node: %d" % (targetHeadBlockNum))
        if targetHeadBlockNum == -1:
            return False

        currentTimeout=timeout-(time.time()-startTime)
        return self.waitOnClusterBlockNumSync(targetHeadBlockNum, currentTimeout)

    def waitOnClusterBlockNumSync(self, targetHeadBlockNum, timeout=None):
        if timeout is None:
            timeout=Utils.systemWaitTimeout
        startTime=time.time()
        remainingTime=timeout
        Utils.Debug and Utils.Print("cmd: remaining time %d seconds" % (remainingTime))
        while time.time()-startTime < timeout:
            synced=True
            for node in self.nodes:
                if node.alive:
                    if node.doesNodeHaveBlockNum(targetHeadBlockNum) is False:
                        synced=False
                        break

            if synced is True:
                return True
            #Utils.Debug and Utils.Print("Brief pause to allow nodes to catch up.")
            sleepTime=3 if remainingTime > 3 else (3 - remainingTime)
            remainingTime -= sleepTime
            Utils.Debug and Utils.Print("cmd: remaining time %d seconds" % (remainingTime))
            time.sleep(sleepTime)

        return False

    @staticmethod
    def createAccountKeys(count):
        accounts=[]
        p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
        for i in range(0, count):
            try:
                cmd="%s create key" % (Utils.EosClientPath)
                Utils.Debug and Utils.Print("cmd: %s" % (cmd))
                keyStr=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
                m=p.match(keyStr)
                if m is None:
                    Utils.Print("ERROR: Owner key creation regex mismatch")
                    break

                ownerPrivate=m.group(1)
                ownerPublic=m.group(2)

                cmd="%s create key" % (Utils.EosClientPath)
                Utils.Debug and Utils.Print("cmd: %s" % (cmd))
                keyStr=subprocess.check_output(cmd.split(), stderr=subprocess.STDOUT).decode("utf-8")
                m=p.match(keyStr)
                if m is None:
                    Utils.Print("ERROR: Owner key creation regex mismatch")
                    break

                activePrivate=m.group(1)
                activePublic=m.group(2)

                name=''.join(random.choice(string.ascii_lowercase) for _ in range(5))
                account=Account(name)
                account.ownerPrivateKey=ownerPrivate
                account.ownerPublicKey=ownerPublic
                account.activePrivateKey=activePrivate
                account.activePublicKey=activePublic
                accounts.append(account)

            except subprocess.CalledProcessError as ex:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during key creation. %s" % (msg))
                break

        if count != len(accounts):
            Utils.Print("Account keys creation failed. Expected %d, actual: %d" % (count, len(accounts)))
            return None

        return accounts

    # create account keys and import into wallet. Wallet initialization will be user responsibility
    def populateWallet(self, accountsCount, wallet):
        if self.walletMgr is None:
            Utils.Print("ERROR: WalletMgr hasn't been initialized.")
            return False

        accounts=None
        if accountsCount > 0:
            Utils.Print ("Create account keys.")
            accounts = self.createAccountKeys(accountsCount)
            if accounts is None:
                Utils.Print("Account keys creation failed.")
                return False

        Utils.Print("Importing keys for account %s into wallet %s." % (self.initaAccount.name, wallet.name))
        if not self.walletMgr.importKey(self.initaAccount, wallet):
            Utils.Print("ERROR: Failed to import key for account %s" % (self.initaAccount.name))
            return False

        Utils.Print("Importing keys for account %s into wallet %s." % (self.initbAccount.name, wallet.name))
        if not self.walletMgr.importKey(self.initbAccount, wallet):
            Utils.Print("ERROR: Failed to import key for account %s" % (self.initbAccount.name))
            return False

        for account in accounts:
            Utils.Print("Importing keys for account %s into wallet %s." % (account.name, wallet.name))
            if not self.walletMgr.importKey(account, wallet):
                Utils.Print("ERROR: Failed to import key for account %s" % (account.name))
                return False

        self.accounts=accounts
        return True

    def getNode(self, id=0):
        return self.nodes[id]

    def getNodes(self):
        return self.nodes

    # Spread funds across accounts with transactions spread through cluster nodes.
    #  Validate transactions are synchronized on root node
    def spreadFunds(self, amount=1):
        Utils.Print("len(self.accounts): %d" % (len(self.accounts)))
        if len(self.accounts) == 0:
            return True

        count=len(self.accounts)
        transferAmount=(count*amount)+amount
        node=self.nodes[0]
        fromm=self.initaAccount
        to=self.accounts[0]
        Utils.Print("Transfer %d units from account %s to %s on eos server port %d" % (
            transferAmount, fromm.name, to.name, node.port))
        trans=node.transferFunds(fromm, to, transferAmount)
        transId=Node.getTransId(trans)
        if transId is None:
            return False
        self.__lastTrans=transId
        Utils.Debug and Utils.Print("Funds transfered on transaction id %s." % (transId))
        self.accounts[0].balance += transferAmount

        nextEosIdx=-1
        for i in range(0, count):
            account=self.accounts[i]
            nextInstanceFound=False
            for n in range(0, count):
                #Utils.Print("nextEosIdx: %d, n: %d" % (nextEosIdx, n))
                nextEosIdx=(nextEosIdx + 1)%count
                if self.nodes[nextEosIdx].alive:
                    #Utils.Print("nextEosIdx: %d" % (nextEosIdx))
                    nextInstanceFound=True
                    break

            if nextInstanceFound is False:
                Utils.Print("ERROR: No active nodes found.")
                return False

            #Utils.Print("nextEosIdx: %d, count: %d" % (nextEosIdx, count))
            node=self.nodes[nextEosIdx]
            Utils.Debug and Utils.Print("Wait for trasaction id %s on node port %d" % (transId, node.port))
            if node.waitForTransIdOnNode(transId) is False:
                Utils.Print("ERROR: Selected node never received transaction id %s" % (transId))
                return False

            transferAmount -= amount
            fromm=account
            to=self.accounts[i+1] if i < (count-1) else self.initaAccount
            Utils.Print("Transfer %d units from account %s to %s on eos server port %d." %
                    (transferAmount, fromm.name, to.name, node.port))

            trans=node.transferFunds(fromm, to, transferAmount)
            transId=Node.getTransId(trans)
            if transId is None:
                return False
            self.__lastTrans=transId
            Utils.Debug and Utils.Print("Funds transfered on block num %s." % (transId))

            self.accounts[i].balance -= transferAmount
            if i < (count-1):
                 self.accounts[i+1].balance += transferAmount

        # As an extra step wait for last transaction on the root node
        node=self.nodes[0]
        Utils.Debug and Utils.Print("Wait for trasaction id %s on node port %d" % (transId, node.port))
        if node.waitForTransIdOnNode(transId) is False:
            Utils.Print("ERROR: Selected node never received transaction id %s" % (transId))
            return False

        return True

    def validateSpreadFunds(self, expectedTotal):
        for node in self.nodes:
            if node.alive:
                Utils.Debug and Utils.Print("Validate funds on %s server port %d." %
                                            (Utils.EosServerName, node.port))
                if node.validateSpreadFundsOnNode(self.initaAccount, self.accounts, expectedTotal) is False:
                    Utils.Print("ERROR: Failed to validate funds on eos node port: %d" % (node.port))
                    return False

        return True

    def spreadFundsAndValidate(self, amount=1):
        Utils.Debug and Utils.Print("Get system balance.")
        initialFunds=self.nodes[0].getSystemBalance(self.initaAccount, self.accounts)
        Utils.Debug and Utils.Print("Initial system balance: %d" % (initialFunds))

        if False == self.spreadFunds(amount):
            Utils.Print("ERROR: Failed to spread funds across nodes.")
            return False

        Utils.Print("Funds spread across all accounts")

        Utils.Print("Validate funds.")
        if False == self.validateSpreadFunds(initialFunds):
            Utils.Print("ERROR: Failed to validate funds transfer across nodes.")
            return False

        return True

    # create account, verify account and return transaction id
    def createAccountAndVerify(self, account, creator, stakedDeposit=1000):
        if len(self.nodes) == 0:
            Utils.Print("ERROR: No nodes initialized.")
            return None
        node=self.nodes[0]

        transId=node.createAccount(account, creator, stakedDeposit)

        if transId is not None and node.verifyAccount(account) is not None:
            return transId
        return None

    @staticmethod
    def parseProducerKeys(configFile):
        """Parse config file. Returns dictionary with account name keys. Dictionary values are list containing the public/private keys"""

        configStr=None
        with open(configFile, 'r') as f:
            configStr=f.read()

        pattern="^\s*private-key\s*=\W+(\w+)\W+(\w+)\W+$"
        m=re.search(pattern, configStr, re.MULTILINE)
        if m is None:
            Utils.Debug and Utils.Print("Failed to find producer keys")
            return None

        pubKey=m.group(1)
        privateKey=m.group(2)

        pattern="^\s*producer-name\s*=\W*(\w+)\W*$"
        matches=re.findall(pattern, configStr, re.MULTILINE)
        if matches is None:
            Utils.Debug and Utils.Print("Failed to find producers.")
            return None

        producerKeys={}
        for m in matches:
            Utils.Debug and Utils.Print ("Found producer : %s" % (m))
            keys=[pubKey, privateKey]
            producerKeys[m]=keys

        return producerKeys

    def parseClusterInfo(self, totalNodes):
        """Parse cluster config file. Updates producer keys data members. Returns True/False"""

        configFile="etc/eosio/node_bios/config.ini"
        Utils.Debug and Utils.Print("Parsing config file %s" % configFile)
        producerKeys=Cluster.parseProducerKeys(configFile)
        if producerKeys is None:
            Utils.Print("ERROR: Failed to parse eosio private keys from cluster config files.")
            return False

        for i in range(0, totalNodes):
            configFile="etc/eosio/node_%02d/config.ini" % (i)
            Utils.Debug and Utils.Print("Parsing config file %s" % configFile)

            keys=Cluster.parseProducerKeys(configFile)
            if keys is not None:
                producerKeys.update(keys)

        initaKeys=producerKeys["inita"]
        initbKeys=producerKeys["initb"]
        eosioKeys=producerKeys["eosio"]
        if initaKeys is None or initbKeys is None:
            Utils.Print("ERROR: Failed to parse inita or intb private keys from cluster config files.")
            return False
        self.initaAccount.ownerPrivateKey=initaKeys[1]
        self.initaAccount.ownerPublicKey=initaKeys[0]
        self.initaAccount.activePrivateKey=initaKeys[1]
        self.initaAccount.activePublicKey=initaKeys[0]
        self.initbAccount.ownerPrivateKey=initbKeys[1]
        self.initbAccount.ownerPublicKey=initbKeys[0]
        self.initbAccount.activePrivateKey=initbKeys[1]
        self.initbAccount.activePublicKey=initbKeys[0]
        self.eosioAccount.ownerPrivateKey=eosioKeys[1]
        self.eosioAccount.ownerPublicKey=eosioKeys[0]
        self.eosioAccount.activePrivateKey=eosioKeys[1]
        self.eosioAccount.activePublicKey=eosioKeys[0]

        self.producerKeys=producerKeys
        return True

    # Populates list of EosInstanceInfo objects, matched to actual running instances
    def discoverLocalNodes(self, totalNodes):
        nodes=[]

        try:
            pgrepOpts="-fl"
            if platform.linux_distribution()[0] == "Ubuntu":
                pgrepOpts="-a"
            elif platform.linux_distribution()[0] == "LinuxMint":
                pgrepOpts="-a"

            cmd="pgrep %s %s" % (pgrepOpts, Utils.EosServerName)
            Utils.Debug and Utils.Print("cmd: %s" % (cmd))
            psOut=subprocess.check_output(cmd.split()).decode("utf-8")
            #Utils.Print("psOut: <%s>" % psOut)

            for i in range(0, totalNodes):
                pattern="[\n]?(\d+) (.* --data-dir var/lib/node_%02d)" % (i)
                m=re.search(pattern, psOut, re.MULTILINE)
                if m is None:
                    Utils.Print("ERROR: Failed to find %s pid. Pattern %s" % (Utils.EosServerName, pattern))
                    break
                instance=Node(self.host, self.port + i, pid=int(m.group(1)), cmd=m.group(2), alive=True, enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
                instance.setWalletEndpointArgs(self.walletEndpointArgs)
                Utils.Debug and Utils.Print("Node:", instance)
                nodes.append(instance)
        except subprocess.CalledProcessError as ex:
            Utils.Print("ERROR: Exception during Nodes creation:", ex)
            raise

        return nodes

    # Check state of running nodeos process and update EosInstanceInfos
    #def updateEosInstanceInfos(eosInstanceInfos):
    def updateNodesStatus(self):
        for node in self.nodes:
            node.checkPulse()

    # Kills a percentange of Eos instances starting from the tail and update eosInstanceInfos state
    def killSomeEosInstances(self, killCount, killSignalStr=Utils.SigKillTag):
        killSignal=signal.SIGKILL
        if killSignalStr == Utils.SigTermTag:
            killSignal=signal.SIGTERM
        Utils.Print("Kill %d %s instances with signal %s." % (killCount, Utils.EosServerName, killSignal))

        killedCount=0
        for node in reversed(self.nodes):
            try:
                os.kill(node.pid, killSignal)
            except Exception as ex:
                Utils.Print("ERROR: Failed to kill process pid %d." % (node.pid), ex)
                return False
            killedCount += 1
            if killedCount >= killCount:
                break

        time.sleep(1) # Give processes time to stand down
        return self.updateNodesStatus()

    def relaunchEosInstances(self):

        chainArg=self.__chainSyncStrategy.arg

        for i in range(0, len(self.nodes)):
            node=self.nodes[i]
            running=True
            try:
                os.kill(node.pid, 0) #check if instance is running
            except Exception as ex:
                running=False

            if running is False:
                dataDir="var/lib/node_%02d" % (i)
                dt = datetime.datetime.now()
                dateStr="%d_%02d_%02d_%02d_%02d_%02d" % (
                    dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)
                stdoutFile="%s/stdout.%s.txt" % (dataDir, dateStr)
                stderrFile="%s/stderr.%s.txt" % (dataDir, dateStr)
                with open(stdoutFile, 'w') as sout, open(stderrFile, 'w') as serr:
                    cmd=node.cmd + ("" if chainArg is None else (" " + chainArg))
                    Utils.Print("cmd: %s" % (cmd))
                    popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
                    self.nodes[i].pid=popen.pid

        return self.updateNodesStatus()

    def dumpErrorDEtailImpl(self,fileName):
        Utils.Print("=================================================================")
        Utils.Print("Contents of %s:" % (fileName))
        with open(fileName, "r") as f:
            shutil.copyfileobj(f, sys.stdout)

    def dumpErrorDetails(self):
        for i in range(0, len(self.nodes)):
            fileName="etc/eosio/node_$02d/config.ini" % (i)
            dumpErrorDetailImpl(filenme)
            fileName="var/lib/node_%02d/stderr.txt" % (i)
            dumpErrorDetailImpl(filenme)

    def killall(self, silent=True):
        cmd="%s -k 15" % (Utils.EosLauncherPath)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            not silent and Utils.Print("Launcher failed to shut down eos cluster.")

        # ocassionally the launcher cannot kill the eos server
        cmd="pkill %s" % (Utils.EosServerName)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            not silent and Utils.Print("Failed to shut down eos cluster.")

        # another explicit nodes shutdown
        for node in self.nodes:
            try:
                os.kill(node.pid, signal.SIGKILL)
            except:
                pass

    def waitForNextBlock(self, timeout=None):
        if timeout is None:
            timeout=Utils.systemWaitTimeout
        node=self.nodes[0]
        return node.waitForNextBlock(timeout)

    def cleanup(self):
        for f in glob.glob("var/lib/node_*"):
            shutil.rmtree(f)
        for f in glob.glob("etc/eosio/node_*"):
            shutil.rmtree(f)

        if self.enableMongo:
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand="db.dropDatabase()"
            Utils.Debug and Utils.Print("echo %s | %s" % (subcommand, cmd))
            ret,outs,errs=Node.stdinAndCheckOutput(cmd.split(), subcommand)
            if ret is not 0:
                Utils.Print("ERROR: Failed to drop database: %s" % (Node.byteArrToStr(errs)) )


    # Create accounts and validates that the last transaction is received on root node
    def createAccounts(self, creator, waitForTransBlock=True, stakedDeposit=1000):
        if self.accounts is None:
            return True

        transId=None
        for account in self.accounts:
            Utils.Debug and Utils.Print("Create account %s." % (account.name))
            trans=self.createAccountAndVerify(account, creator, stakedDeposit)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s." % (account.name))
                return False
            Utils.Debug and Utils.Print("Account %s created." % (account.name))
            transId=Node.getTransId(trans)

        if waitForTransBlock and transId is not None:
            node=self.nodes[0]
            Utils.Debug and Utils.Print("Wait for transaction id %s on server port %d." % ( transId, node.port))
            if node.waitForTransIdOnNode(transId) is False:
                Utils.Print("ERROR: Failed waiting for transaction id %s on server port %d." % (
                    transId, node.port))
                return False

        return True
###########################################################################################
