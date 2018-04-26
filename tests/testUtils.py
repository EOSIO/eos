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

    EosWalletName="keosd"
    EosWalletPath="programs/keosd/"+ EosWalletName

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

    @staticmethod
    def waitForObj(lam, timeout=None):
        if timeout is None:
            timeout=60

        endTime=time.time()+timeout
        while endTime > time.time():
            ret=lam()
            if ret is not None:
                return ret
            sleepTime=3
            Utils.Print("cmd: sleep %d seconds, remaining time: %d seconds" %
                        (sleepTime, endTime - time.time()))
            time.sleep(sleepTime)

        return None

    @staticmethod
    def waitForBool(lam, timeout=None):
        myLam = lambda: True if lam() else None
        ret=Utils.waitForObj(myLam)
        return False if ret is None else ret


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
        return "Name: %s" % (self.name)

###########################################################################################
class Node(object):

    def __init__(self, host, port, pid=None, cmd=None, enableMongo=False, mongoHost="localhost", mongoPort=27017, mongoDb="EOStest"):
        self.host=host
        self.port=port
        self.pid=pid
        self.cmd=cmd
        self.killed=False # marks node as killed
        self.enableMongo=enableMongo
        self.mongoSyncTime=None if Utils.mongoSyncTime < 1 else Utils.mongoSyncTime
        self.mongoHost=mongoHost
        self.mongoPort=mongoPort
        self.mongoDb=mongoDb
        self.endpointArgs="--url http://%s:%d" % (self.host, self.port)
        self.mongoEndpointArgs=""
        if self.enableMongo:
            self.mongoEndpointArgs += "--host %s --port %d %s" % (mongoHost, mongoPort, mongoDb)

    def __str__(self):
        #return "Host: %s, Port:%d, Pid:%s, Cmd:\"%s\"" % (self.host, self.port, self.pid, self.cmd)
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
        ret=0
        try:
            popen=subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            outs,errs=popen.communicate(input=subcommand.encode("utf-8"))
            ret=popen.wait()
        except subprocess.CalledProcessError as ex:
            msg=ex.output
            return (ex.returncode, msg, None)

        return (ret, outs, errs)

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
    def getTransId_nj(trans):
        """Retrieve transaction id from cleos non-json output."""
        # parse out transaction id
        pattern="executed transaction:\s+(\w+)\s+"
        m=re.search(pattern, trans, re.MULTILINE)
        assert(m is not None)
        return m.group(1)

    @staticmethod
    def getTransId(trans):
        """Retrieve transaction id from dictionary object."""
        # TBD: add assert for trans is Dictionary
        #Utils.Print("%s" % trans)
        transId=trans["transaction_id"]
        return transId

    @staticmethod
    def byteArrToStr(arr):
        return arr.decode("utf-8")

    def setWalletEndpointArgs(self, args):
        self.endpointArgs="--url http://%s:%d %s" % (self.host, self.port, args)

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
        if info is None:
            return False
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
                msg=ex.output.decode("utf-8")
                if "Failed to connect" in msg:
                    Utils.Print("ERROR: Node is unreachable. %s" % (msg))
                    raise
                if not silentErrors:
                    Utils.Print("ERROR: Exception during transaction retrieval. %s" % (msg))
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
            blockNum=int(trans["transaction"]["transaction"]["ref_block_num"])
        else:
            blockNum=int(trans["ref_block_num"])

        blockNum += 1
        Utils.Debug and Utils.Print("Check if block %d is irreversible." % (blockNum))
        return self.doesNodeHaveBlockNum(blockNum)

    # Create account and return creation transactions. Return transaction json object
    # waitForTransBlock: wait on creation transaction id to appear in a block
    def createAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False):
        cmd="%s %s create account -j %s %s %s %s" % (
            Utils.EosClientPath, self.endpointArgs, creatorAccount.name, account.name,
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

        if stakedDeposit > 0:
            self.waitForTransIdOnNode(transId) # seems like account creation needs to be finlized before transfer can happen
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
        lam = lambda: self.doesNodeHaveBlockNum(blockNum)
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForTransIdOnNode(self, transId, timeout=None):
        lam = lambda: self.doesNodeHaveTransId(transId)
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForNextBlock(self, timeout=None):
        num=self.getIrreversibleBlockNum()
        lam = lambda: self.getIrreversibleBlockNum() > num
        ret=Utils.waitForBool(lam, timeout)
        return ret

    # Trasfer funds. Returns "transfer" json return object
    def transferFunds(self, source, destination, amount, memo="memo", force=False):
        cmd="%s %s -v transfer -j %s %s %d" % (
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
            Utils.Debug and Utils.Print("getEosCurrencyBalance %s %s", name, amount)
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
        cmd="%s %s get transactions -j %s" % (Utils.EosClientPath, self.endpointArgs, name)
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
    def publishContract(self, account, contractDir, wastFile, abiFile, waitForTransBlock=False, shouldFail=False):
        cmd="%s %s -v set contract -j %s %s" % (Utils.EosClientPath, self.endpointArgs, account, contractDir)
        cmd += "" if wastFile is None else (" "+ wastFile)
        cmd += "" if abiFile is None else (" " + abiFile)
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

    def getTable(self, contract, scope, table):
        cmd="%s %s get table %s %s %s" % (Utils.EosClientPath, self.endpointArgs, contract, scope, table)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Node.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during table retrieval. %s" % (msg))
            return None

    def getTableRows(self, contract, scope, table):
        jsonData=self.getTable(contract, scope, table)
        if jsonData is None:
            return None
        rows=jsonData["rows"]
        return rows

    def getTableRow(self, contract, scope, table, idx):
        if idx < 0:
            Utils.Print("ERROR: Table index cannot be negative. idx: %d" % (idx))
            return None
        rows=self.getTableRows(contract, scope, table)
        if rows is None or idx >= len(rows):
            Utils.Print("ERROR: Retrieved table does not contain row %d" % idx)
            return None
        row=rows[idx]
        return row

    def getTableColumns(self, contract, scope, table):
        row=self.getTableRow(contract, scope, table, 0)
        keys=list(row.keys())
        return keys

    # returns tuple with transaction and
    def pushMessage(self, contract, action, data, opts, silentErrors=False):
        cmd="%s %s push action -j %s %s" % (Utils.EosClientPath, self.endpointArgs, contract, action)
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
        cmd="%s %s set action permission -j %s %s %s %s" % (
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
            return True
        else:
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

    def kill(self, killSignal):
        try:
            Utils.Debug and Utils.Print("Killing node: %s" % (self.cmd))
            os.kill(self.pid, killSignal)

            # wait for kill validation
            def myFunc():
                try:
                    os.kill(self.pid, 0) #check if process with pid is running
                except Exception as ex:
                    return True
                return False

            lam = lambda: myFunc()
            if not Utils.waitForBool(lam):
                Utils.Print("ERROR: Failed to kill node (%s)." % (self.cmd), ex)
                return False

            # mark node as killed
            self.killed=True
            return True
        except Exception as ex:
            Utils.Print("ERROR: Failed to kill node (%d)." % (self.cmd), ex)

        return False

    # TBD: make nodeId an internal property
    def relaunch(self, nodeId, chainArg):

        running=True
        try:
            os.kill(self.pid, 0) #check if process with pid is running
        except Exception as ex:
            running=False

        if running:
            Utils.Print("WARNING: A process with pid (%d) is already running." % (self.pid))
        else:
            Utils.Debug and Utils.Print("Launching node process, Id: %d" % (nodeId))
            dataDir="var/lib/node_%02d" % (nodeId)
            dt = datetime.datetime.now()
            dateStr="%d_%02d_%02d_%02d_%02d_%02d" % (
                dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)
            stdoutFile="%s/stdout.%s.txt" % (dataDir, dateStr)
            stderrFile="%s/stderr.%s.txt" % (dataDir, dateStr)
            with open(stdoutFile, 'w') as sout, open(stderrFile, 'w') as serr:
                cmd=self.cmd + ("" if chainArg is None else (" " + chainArg))
                Utils.Print("cmd: %s" % (cmd))
                popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
                self.pid=popen.pid

        self.killed=False
        return True

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

    
###########################################################################################

Wallet=namedtuple("Wallet", "name password host port")
class WalletMgr(object):
    __walletLogFile="test_keosd_output.log"
    __walletDataDir="test_wallet_0"

    # walletd [True|False] True=Launch wallet(keosd) process; False=Manage launch process externally.
    def __init__(self, walletd, nodeosPort=8888, nodeosHost="localhost", port=8899, host="localhost"):
        self.walletd=walletd
        self.nodeosPort=nodeosPort
        self.nodeosHost=nodeosHost
        self.port=port
        self.host=host
        self.wallets={}
        self.__walletPid=None
        self.endpointArgs="--url http://%s:%d" % (self.nodeosHost, self.nodeosPort)
        self.walletEndpointArgs=""
        if self.walletd:
            self.walletEndpointArgs += " --wallet-url http://%s:%d" % (self.host, self.port)
            self.endpointArgs += self.walletEndpointArgs

    def launch(self):
        if not self.walletd:
            Utils.Print("ERROR: Wallet Manager wasn't configured to launch keosd")
            return False

        cmd="%s --data-dir %s --config-dir %s --http-server-address=%s:%d" % (
            Utils.EosWalletPath, WalletMgr.__walletDataDir, WalletMgr.__walletDataDir, self.host, self.port)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        with open(WalletMgr.__walletLogFile, 'w') as sout, open(WalletMgr.__walletLogFile, 'w') as serr:
            popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
            self.__walletPid=popen.pid

        # Give keosd time to warm up
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
        warningMsg="Key already in wallet"
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
        cmd="pkill -9 %s" % (Utils.EosWalletName)
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
    __BiosHost="localhost"
    __BiosPort=8788
    
    
    # walletd [True|False] Is keosd running. If not load the wallet plugin
    def __init__(self, walletd=False, localCluster=True, host="localhost", port=8888, walletHost="localhost", walletPort=8899, enableMongo=False, mongoHost="localhost", mongoPort=27017, mongoDb="EOStest", initaPrvtKey=None, initbPrvtKey=None, staging=False):
        """Cluster container.
        walletd [True|False] Is wallet keosd running. If not load the wallet plugin
        localCluster [True|False] Is cluster local to host.
        host: eos server host
        port: eos server port
        walletHost: eos wallet host
        walletPort: wos wallet port
        enableMongo: Include mongoDb support, configures eos mongo plugin
        mongoHost: MongoDB host
        mongoPort: MongoDB port
        initaPrvtKey: Inita account private key
        initbPrvtKey: Initb account private key
        """
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
            self.walletEndpointArgs += " --wallet-url http://%s:%d" % (self.walletHost, self.walletPort)
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
        self.initaAccount.activePrivateKey=initaPrvtKey
        self.initbAccount.ownerPrivateKey=initbPrvtKey
        self.initbAccount.activePrivateKey=initbPrvtKey


    def setChainStrategy(self, chainSyncStrategy=Utils.SyncReplayTag):
        self.__chainSyncStrategy=self.__chainSyncStrategies.get(chainSyncStrategy)
        if self.__chainSyncStrategy is None:
            self.__chainSyncStrategy= __chainSyncStrategies.get("none")

    def setWalletMgr(self, walletMgr):
        self.walletMgr=walletMgr

    # launch local nodes and set self.nodes
    def launch(self, pnodes=1, totalNodes=1, prodCount=1, topo="mesh", delay=1):
        """Launch cluster.
        pnodes: producer nodes count
        totalNodes: producer + non-producer nodes count
        prodCount: producers per prodcuer node count
        topo: cluster topology (as defined by launcher)
        delay: delay between individual nodes launch (as defined by launcher)
          delay 0 exposes a bootstrap bug where producer handover may have a large gap confusing nodes and bringing system to a halt.
        """
        if not self.localCluster:
            Utils.Print("WARNING: Cluster not local, not launching %s." % (Utils.EosServerName))
            return True

        if len(self.nodes) > 0:
            raise RuntimeError("Cluster already running.")

        cmd="%s -p %s -n %s -s %s -d %s -f" % (
            Utils.EosLauncherPath, pnodes, totalNodes, topo, delay)
        cmdArr=cmd.split()
        if self.staging:
            cmdArr.append("--nogen")

        nodeosArgs=""
        if not self.walletd:
            nodeosArgs += " --plugin eosio::wallet_api_plugin"
        if self.enableMongo:
            nodeosArgs += " --plugin eosio::mongo_db_plugin --resync --mongodb-uri %s" % self.mongoUri

        if nodeosArgs:
            cmdArr.append("--nodeos")
            cmdArr.append(nodeosArgs)

        s=" ".join(cmdArr)
        Utils.Debug and Utils.Print("cmd: %s" % (s))
        if 0 != subprocess.call(cmdArr):
            Utils.Print("ERROR: Launcher failed to launch.")
            return False

        self.nodes=range(totalNodes) # placeholder for cleanup purposes only

        nodes=self.discoverLocalNodes(totalNodes, timeout=Utils.systemWaitTimeout)
        if nodes is None or totalNodes != len(nodes):
            Utils.Print("ERROR: Unable to validate %s instances, expected: %d, actual: %d" %
                          (Utils.EosServerName, totalNodes, len(nodes)))
            return False

        self.nodes=nodes

        # ensure cluster node are inter-connected by ensuring everyone has block 1
        Utils.Print("Cluster viability smoke test. Validate every cluster node has block 1. ")
        if not self.waitOnClusterBlockNumSync(1):
            Utils.Print("ERROR: Cluster doesn't seem to be in sync. Some nodes missing block 1")
            return False

        Utils.Print("Bootstrap cluster.")
        if not Cluster.bootstrap(totalNodes, prodCount, Cluster.__BiosHost, Cluster.__BiosPort):
            Utils.Print("ERROR: Bootstrap failed.")
            return False

        producerKeys=Cluster.parseClusterKeys(totalNodes)
        if producerKeys is None:
            Utils.Print("ERROR: Unable to parse cluster info")
            return False

        init1Keys=producerKeys["inita"]
        init2Keys=producerKeys["initb"]
        if init1Keys is None or init2Keys is None:
            Utils.Print("ERROR: Failed to parse inita or intb private keys from cluster config files.")
        self.initaAccount.ownerPrivateKey=init1Keys["private"]
        self.initaAccount.ownerPublicKey=init1Keys["public"]
        self.initaAccount.activePrivateKey=init1Keys["private"]
        self.initaAccount.activePublicKey=init1Keys["public"]
        self.initbAccount.ownerPrivateKey=init2Keys["private"]
        self.initbAccount.ownerPublicKey=init2Keys["public"]
        self.initbAccount.activePrivateKey=init2Keys["private"]
        self.initbAccount.activePublicKey=init2Keys["public"]
        producerKeys.pop("eosio")

        return True

    # Initialize the default nodes (at present just the root node)
    def initializeNodes(self, initaPrvtKey=None, initbPrvtKey=None):
        node=Node(self.host, self.port, enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
        node.setWalletEndpointArgs(self.walletEndpointArgs)
        Utils.Debug and Utils.Print("Node:", node)

        node.checkPulse()
        self.nodes=[node]

        if initaPrvtKey is not None:
            self.initaAccount.ownerPrivateKey=initaPrvtKey
            self.initaAccount.activePrivateKey=initaPrvtKey

        if initbPrvtKey is not None:
            self.initbAccount.ownerPrivateKey=initbPrvtKey
            self.initbAccount.activePrivateKey=initbPrvtKey

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
        targetHeadBlockNum=self.nodes[0].getHeadBlockNum() #get root nodes head block num
        Utils.Debug and Utils.Print("Head block number on root node: %d" % (targetHeadBlockNum))
        if targetHeadBlockNum == -1:
            return False

        return self.waitOnClusterBlockNumSync(targetHeadBlockNum)

    def waitOnClusterBlockNumSync(self, targetHeadBlockNum, timeout=None):

        def doNodesHaveBlockNum(nodes, targetHeadBlockNum):
            for node in nodes:
                if (not node.killed) and (not node.doesNodeHaveBlockNum(targetHeadBlockNum)):
                    return False

            return True

        lam = lambda: doNodesHaveBlockNum(self.nodes, targetHeadBlockNum)
        ret=Utils.waitForBool(lam, timeout)
        return ret

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
                Utils.Debug and Utils.Print("name: %s, key: ['%s', '%s]-owner; ['%s', '%s']-active" % (name, ownerPublic, ownerPrivate, activePublic, activePrivate))

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
                if not self.nodes[nextEosIdx].killed:
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
            if not node.killed:
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
    def nodeNameToId(name):
        """Convert node name to decimal id. Node name regex is "node_([\d]+)". "node_bios" is a special name which returns -1. Examples: node_00 => 0, node_21 => 21, node_bios => -1. """
        if name == "node_bios":
            return -1

        m=re.search("node_([\d]+)", name)
        return int(m.group(1))

        
    @staticmethod
    def parseProducerKeys(configFile, nodeName):
        """Parse node config file for producer keys. Returns dictionary. (Keys: account name; Values: dictionary objects (Keys: ["name", "node", "private","public"]; Values: account name, node id returned by nodeNameToId(nodeName), private key(string)and public key(string)))."""

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
            nodeId=Cluster.nodeNameToId(nodeName)
            keys={"name": m, "node": nodeId, "private": privateKey, "public": pubKey}
            producerKeys[m]=keys

        return producerKeys

    @staticmethod
    def parseClusterKeys(totalNodes):
        """Parse cluster config file. Updates producer keys data members."""

        node="node_bios"
        configFile="etc/eosio/%s/config.ini" % (node)
        Utils.Debug and Utils.Print("Parsing config file %s" % configFile)
        producerKeys=Cluster.parseProducerKeys(configFile, node)
        if producerKeys is None:
            Utils.Print("ERROR: Failed to parse eosio private keys from cluster config files.")
            return None

        for i in range(0, totalNodes):
            node="node_%02d" % (i)
            configFile="etc/eosio/%s/config.ini" % (node)
            Utils.Debug and Utils.Print("Parsing config file %s" % configFile)

            keys=Cluster.parseProducerKeys(configFile, node)
            if keys is not None:
                producerKeys.update(keys)

        return producerKeys

    @staticmethod
    def bootstrap(totalNodes, prodCount, biosHost, biosPort):
        """Create 'prodCount' init accounts and deposits 10000000000 EOS in each. If prodCount is -1 will initialize all possible producers.
        Ensure nodes are inter-connected prior to this call. One way to validate this will be to check if every node has block 1."""

        Utils.Print("Starting cluster bootstrap.")
        biosNode=Node(biosHost, biosPort)
        if not biosNode.checkPulse():
            Utils.Print("ERROR: Bios node doesn't appear to be running...")
            return False

        producerKeys=Cluster.parseClusterKeys(totalNodes)
        # should have totalNodes node plus bios node
        if producerKeys is None or len(producerKeys) < (totalNodes+1):
            Utils.Print("ERROR: Failed to parse private keys from cluster config files.")
            return False

        walletMgr=WalletMgr(True)
        if not walletMgr.launch():
            Utils.Print("ERROR: Failed to launch bootstrap wallet.")
            return False
        biosNode.setWalletEndpointArgs(walletMgr.walletEndpointArgs)

        try:
            ignWallet=walletMgr.create("ignition")
            if ignWallet is None:
                Utils.Print("ERROR: Failed to create ignition wallet.")
                return False

            eosioName="eosio"
            eosioKeys=producerKeys[eosioName]
            eosioAccount=Account(eosioName)
            eosioAccount.ownerPrivateKey=eosioKeys["private"]
            eosioAccount.ownerPublicKey=eosioKeys["public"]
            eosioAccount.activePrivateKey=eosioKeys["private"]
            eosioAccount.activePublicKey=eosioKeys["public"]

            if not walletMgr.importKey(eosioAccount, ignWallet):
                Utils.Print("ERROR: Failed to import %s account keys into ignition wallet." % (eosioName))
                return False

            contract="eosio.bios"
            contractDir="contracts/%s" % (contract)
            wastFile="contracts/%s/%s.wast" % (contract, contract)
            abiFile="contracts/%s/%s.abi" % (contract, contract)
            Utils.Print("Publish %s contract" % (contract))
            trans=biosNode.publishContract(eosioAccount.name, contractDir, wastFile, abiFile, waitForTransBlock=True)
            if trans is None:
                Utils.Print("ERROR: Failed to publish contract %s." % (contract))
                return False

            Utils.Print("Creating accounts: %s " % ", ".join(producerKeys.keys()))
            producerKeys.pop(eosioName)
            for name, keys in producerKeys.items():
                initx = Account(name)
                initx.ownerPrivateKey=keys["private"]
                initx.ownerPublicKey=keys["public"]
                initx.activePrivateKey=keys["private"]
                initx.activePublicKey=keys["public"]
                trans=biosNode.createAccount(initx, eosioAccount, 0)
                if trans is None:
                    Utils.Print("ERROR: Failed to create account %s" % (name))
                    return False
            transId=Node.getTransId(trans)
            biosNode.waitForTransIdOnNode(transId)

            if prodCount == -1:
                setProdsFile="setprods.json"
                Utils.Debug and Utils.Print("Reading in setprods file %s." % (setProdsFile))
                with open(setProdsFile, "r") as f:
                    setProdsStr=f.read()

                    Utils.Print("Setting producers.")
                    opts="--permission eosio@active"
                    trans=biosNode.pushMessage("eosio", "setprods", setProdsStr, opts)
                    if trans is None or not trans[0]:
                        Utils.Print("ERROR: Failed to set producers.")
                        return False
            else:
                counts=dict.fromkeys(range(totalNodes), 0) #initialize node prods count to 0
                setProdsStr='{ "version": 1, "producers": ['
                firstTime=True
                prodNames=[]
                for name, keys in producerKeys.items():
                    if counts[keys["node"]] >= prodCount:
                        continue
                    if firstTime:
                        firstTime = False
                    else:
                        setProdsStr += ','

                    setProdsStr += ' { "producer_name": "%s", "block_signing_key": "%s" }' % (keys["name"], keys["public"])
                    prodNames.append(keys["name"])
                    counts[keys["node"]] += 1

                setProdsStr += ' ] }'
                Utils.Debug and Utils.Print("setprods: %s" % (setProdsStr))
                Utils.Print("Setting producers: %s." % (", ".join(prodNames)))
                opts="--permission eosio@active"
                trans=biosNode.pushMessage("eosio", "setprods", setProdsStr, opts)
                if trans is None or not trans[0]:
                    Utils.Print("ERROR: Failed to set producer %s." % (keys["name"]))
                    return False

            trans=trans[1]
            transId=Node.getTransId(trans)
            if not biosNode.waitForTransIdOnNode(transId):
                return False

            # wait for block production handover (essentially a block produced by anyone but eosio).
            lam = lambda: biosNode.getInfo()["head_block_producer"] != "eosio"
            ret=Utils.waitForBool(lam)
            if not ret:
                Utils.Print("ERROR: Block production handover failed.")
                return False

            contract="eosio.system"
            contractDir="contracts/%s" % (contract)
            wastFile="contracts/%s/%s.wast" % (contract, contract)
            abiFile="contracts/%s/%s.abi" % (contract, contract)
            Utils.Print("Publish %s contract" % (contract))
            trans=biosNode.publishContract(eosioAccount.name, contractDir, wastFile, abiFile, waitForTransBlock=True)
            if trans is None:
                Utils.Print("ERROR: Failed to publish contract %s." % (contract))
                return False

            # TBD: Create currency, followed by issue currency
            
            Utils.Print("push issue action to eosio contract")
            contract=eosioAccount.name
            action="issue"
            data="{\"to\":\"eosio\",\"quantity\":\"1000000000.0000 EOS\"}"
            opts="--permission eosio@active"
            trans=biosNode.pushMessage(contract, action, data, opts)
            if trans is None or not trans[0]:
                Utils.Print("ERROR: Failed to push issue action to eosio contract.")
                return False

            Utils.Print("Wait for issue action transaction to become finalized.")
            transId=Node.getTransId(trans[1])
            biosNode.waitForTransIdOnNode(transId)

            # TBD: Known issue (Issue 2043) that 'get currency balance' doesn't return balance.
            #  Uncomment when functional
            # expectedAmount=10000000000000
            # Utils.Print("Verify eosio issue, Expected: %d" % (expectedAmount))
            # actualAmount=biosNode.getAccountBalance(eosioAccount.name)
            # if expectedAmount != actualAmount:
            #     Utils.Print("ERROR: Issue verification failed. Excepted %d, actual: %d" %
            #                 (expectedAmount, actualAmount))
            #     return False

            initialFunds=10000000000
            Utils.Print("Transfer initial fund %d to individual accounts." % (initialFunds))
            trans=None
            for name, keys in producerKeys.items():
                initx = Account(name)
                initx.ownerPrivateKey=keys["private"]
                initx.ownerPublicKey=keys["public"]
                initx.activePrivateKey=keys["private"]
                initx.activePublicKey=keys["public"]
                trans = biosNode.transferFunds(eosioAccount, initx, initialFunds, "init transfer")
                if trans is None:
                    Utils.Print("ERROR: Failed to transfer funds from %s to %s." % (eosioAccount.name, name))
                    return False

            Utils.Print("Wait for last transfer transaction to become finalized.")
            transId=Node.getTransId(trans)
            if not biosNode.waitForTransIdOnNode(transId):
                return False

            Utils.Print("Cluster bootstrap done.")
        finally:
            walletMgr.killall()
            walletMgr.cleanup()

        return True

    
    # Populates list of EosInstanceInfo objects, matched to actual running instances
    def discoverLocalNodes(self, totalNodes, timeout=0):
        nodes=[]

        pgrepOpts="-fl"
        if platform.linux_distribution()[0] in ["Ubuntu", "LinuxMint", "Fedora","CentOS Linux","arch"]:
            pgrepOpts="-a"

        cmd="pgrep %s %s" % (pgrepOpts, Utils.EosServerName)

        def myFunc():
            psOut=None
            try:
                Utils.Debug and Utils.Print("cmd: %s" % (cmd))
                psOut=subprocess.check_output(cmd.split()).decode("utf-8")
                return psOut
            except subprocess.CalledProcessError as ex:
                pass
            return None

        lam = lambda:  myFunc()
        psOut=Utils.waitForObj(lam)
        if psOut is None:
            Utils.Print("ERROR: No nodes discovered.")
            return nodes

        Utils.Debug and Utils.Print("pgrep output: \"%s\"" % psOut)
        for i in range(0, totalNodes):
            pattern="[\n]?(\d+) (.* --data-dir var/lib/node_%02d)" % (i)
            m=re.search(pattern, psOut, re.MULTILINE)
            if m is None:
                Utils.Print("ERROR: Failed to find %s pid. Pattern %s" % (Utils.EosServerName, pattern))
                break
            instance=Node(self.host, self.port + i, pid=int(m.group(1)), cmd=m.group(2), enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
            instance.setWalletEndpointArgs(self.walletEndpointArgs)
            Utils.Debug and Utils.Print("Node>", instance)
            nodes.append(instance)

        return nodes

    # Check state of running nodeos process and update EosInstanceInfos
    #def updateEosInstanceInfos(eosInstanceInfos):
    # def updateNodesStatus(self):
    #     for node in self.nodes:
    #         node.checkPulse()

    # Kills a percentange of Eos instances starting from the tail and update eosInstanceInfos state
    def killSomeEosInstances(self, killCount, killSignalStr=Utils.SigKillTag):
        killSignal=signal.SIGKILL
        if killSignalStr == Utils.SigTermTag:
            killSignal=signal.SIGTERM
        Utils.Print("Kill %d %s instances with signal %s." % (killCount, Utils.EosServerName, killSignal))

        killedCount=0
        for node in reversed(self.nodes):
            if not node.kill(killSignal):
                return False

            killedCount += 1
            if killedCount >= killCount:
                break

        time.sleep(1) # Give processes time to stand down
        # return self.updateNodesStatus()
        return True

    def relaunchEosInstances(self):

        chainArg=self.__chainSyncStrategy.arg

        for i in range(0, len(self.nodes)):
            node=self.nodes[i]
            if not node.relaunch(i, chainArg):
                return False

        return True

    def dumpErrorDetailImpl(self,fileName):
        Utils.Print("=================================================================")
        Utils.Print("Contents of %s:" % (fileName))
        if os.path.exists(fileName):
            with open(fileName, "r") as f:
                shutil.copyfileobj(f, sys.stdout)
        else:
            Utils.Print("File %s not found." % (fileName))

    def dumpErrorDetails(self):
        fileName="etc/eosio/node_bios/config.ini"
        self.dumpErrorDetailImpl(fileName)
        fileName="var/lib/node_bios/stderr.txt"
        self.dumpErrorDetailImpl(fileName)

        for i in range(0, len(self.nodes)):
            fileName="etc/eosio/node_%02d/config.ini" % (i)
            self.dumpErrorDetailImpl(fileName)
            fileName="var/lib/node_%02d/stderr.txt" % (i)
            self.dumpErrorDetailImpl(fileName)

    def killall(self, silent=True):
        cmd="%s -k 15" % (Utils.EosLauncherPath)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            not silent and Utils.Print("Launcher failed to shut down eos cluster.")

        # ocassionally the launcher cannot kill the eos server
        cmd="pkill -9 %s" % (Utils.EosServerName)
        Utils.Debug and Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            not silent and Utils.Print("Failed to shut down eos cluster.")

        # another explicit nodes shutdown
        for node in self.nodes:
            try:
                os.kill(node.pid, signal.SIGKILL)
            except:
                pass

    def isMongodDbRunning(self):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand="db.version()"
        Utils.Debug and Utils.Print("echo %s | %s" % (subcommand, cmd))
        ret,outs,errs=Node.stdinAndCheckOutput(cmd.split(), subcommand)
        if ret is not 0:
            Utils.Print("ERROR: Failed to check database version: %s" % (Node.byteArrToStr(errs)) )
            return False
        Utils.Debug and Utils.Print("MongoDb response: %s" % (outs))
        return True

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
