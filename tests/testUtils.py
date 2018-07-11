import argparse
import copy
import decimal
import errno
import subprocess
import time
import glob
import shutil
import os
import platform
from collections import namedtuple
import re
import string
import signal
import socket
import datetime
import inspect
import sys
import random
import json
import shlex
from sys import stdout

from core_symbol import CORE_SYMBOL

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
        s=' '*stackDepth
        stdout.write(s)
        print(*args, **kwargs)

    SyncStrategy=namedtuple("ChainSyncStrategy", "name id arg")

    SyncNoneTag="none"
    SyncReplayTag="replay"
    SyncResyncTag="resync"
    SyncHardReplayTag="hardReplay"

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

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncResyncTag, 2, "--delete-all-blocks")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncHardReplayTag, 3, "--hard-replay-blockchain")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        return chainSyncStrategies

    @staticmethod
    def checkOutput(cmd):
        assert(isinstance(cmd, list))
        popen=subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (output,error)=popen.communicate()
        if popen.returncode != 0:
            raise subprocess.CalledProcessError(returncode=popen.returncode, cmd=cmd, output=error)
        return output.decode("utf-8")

    @staticmethod
    def errorExit(msg="", raw=False, errorCode=1):
        Utils.Print("ERROR:" if not raw else "", msg)
        exit(errorCode)

    @staticmethod
    def cmdError(name, cmdCode=0, exitNow=False):
        msg="FAILURE - %s%s" % (name, ("" if cmdCode == 0 else (" returned error code %d" % cmdCode)))
        if exitNow:
            Utils.errorExit(msg, True)
        else:
            Utils.Print(msg)

    @staticmethod
    def waitForObj(lam, timeout=None):
        if timeout is None:
            timeout=60

        endTime=time.time()+timeout
        needsNewLine=False
        try:
            while endTime > time.time():
                ret=lam()
                if ret is not None:
                    return ret
                sleepTime=3
                if Utils.Debug:
                    Utils.Print("cmd: sleep %d seconds, remaining time: %d seconds" %
                                (sleepTime, endTime - time.time()))
                else:
                    stdout.write('.')
                    stdout.flush()
                    needsNewLine=True
                time.sleep(sleepTime)
        finally:
            if needsNewLine:
                Utils.Print()

        return None

    @staticmethod
    def waitForBool(lam, timeout=None):
        myLam = lambda: True if lam() else None
        ret=Utils.waitForObj(myLam, timeout)
        return False if ret is None else ret

    @staticmethod
    def filterJsonObject(data):
        firstIdx=data.find('{')
        lastIdx=data.rfind('}')
        retStr=data[firstIdx:lastIdx+1]
        return retStr

    @staticmethod
    def runCmdArrReturnJson(cmdArr, trace=False, silentErrors=True):
        retStr=Utils.checkOutput(cmdArr)
        jStr=Utils.filterJsonObject(retStr)
        if trace: Utils.Print ("RAW > %s"% (retStr))
        if trace: Utils.Print ("JSON> %s"% (jStr))
        if not jStr:
            msg="Received empty JSON response"
            if not silentErrors:
                Utils.Print ("ERROR: "+ msg)
                Utils.Print ("RAW > %s"% retStr)
            raise TypeError(msg)

        try:
            jsonData=json.loads(jStr)
            return jsonData
        except json.decoder.JSONDecodeError as ex:
            Utils.Print (ex)
            Utils.Print ("RAW > %s"% retStr)
            Utils.Print ("JSON> %s"% jStr)
            raise

    @staticmethod
    def runCmdReturnStr(cmd, trace=False):
        retStr=Utils.checkOutput(cmd.split())
        if trace: Utils.Print ("RAW > %s"% (retStr))
        return retStr

    @staticmethod
    def runCmdReturnJson(cmd, trace=False, silentErrors=False):
        cmdArr=shlex.split(cmd)
        return Utils.runCmdArrReturnJson(cmdArr, trace=trace, silentErrors=silentErrors)


###########################################################################################
class Account(object):
    # pylint: disable=too-few-public-methods

    def __init__(self, name):
        self.name=name

        self.ownerPrivateKey=None
        self.ownerPublicKey=None
        self.activePrivateKey=None
        self.activePublicKey=None


    def __str__(self):
        return "Name: %s" % (self.name)

###########################################################################################
# pylint: disable=too-many-public-methods
class Node(object):

    # pylint: disable=too-many-instance-attributes
    # pylint: disable=too-many-arguments
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
    def validateTransaction(trans):
        assert trans
        assert isinstance(trans, dict), print("Input type is %s" % type(trans))

        def printTrans(trans):
            Utils.Print("ERROR: Failure in transaction validation.")
            Utils.Print("Transaction: %s" % (json.dumps(trans, indent=1)))

        assert trans["processed"]["receipt"]["status"] == "executed", printTrans(trans)

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
        retId,outs=Node.stdinAndCheckOutput(cmdArr, subcommand)
        if retId is not 0:
            return None
        outStr=Node.byteArrToStr(outs)
        if not outStr:
            return None
        extJStr=Utils.filterJsonObject(outStr)
        if not extJStr:
            return None
        jStr=Node.normalizeJsonObject(extJStr)
        if not jStr:
            return None
        if trace: Utils.Print ("RAW > %s"% (outStr))
        #trace and Utils.Print ("JSON> %s"% jStr)
        jsonData=json.loads(jStr)
        return jsonData

    @staticmethod
    def getTransId(trans):
        """Retrieve transaction id from dictionary object."""
        assert trans
        assert isinstance(trans, dict), print("Input type is %s" % type(trans))

        #Utils.Print("%s" % trans)
        transId=trans["transaction_id"]
        return transId

    @staticmethod
    def byteArrToStr(arr):
        return arr.decode("utf-8")

    def setWalletEndpointArgs(self, args):
        self.endpointArgs="--url http://%s:%d %s" % (self.host, self.port, args)

    def validateAccounts(self, accounts):
        assert(accounts)
        assert(isinstance(accounts, list))

        for account in accounts:
            assert(account)
            assert(isinstance(account, Account))
            if Utils.Debug: Utils.Print("Validating account %s" % (account.name))
            accountInfo=self.getEosAccount(account.name)
            try:
                assert(accountInfo)
                assert(accountInfo["account_name"] == account.name)
            except (AssertionError, TypeError, KeyError) as _:
                Utils.Print("account validation failed. account: %s" % (account.name))
                raise

    # pylint: disable=too-many-branches
    def getBlock(self, blockNum, retry=True, silentErrors=False):
        """Given a blockId will return block details."""
        assert(isinstance(blockNum, int))
        if not self.enableMongo:
            cmd="%s %s get block %d" % (Utils.EosClientPath, self.endpointArgs, blockNum)
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            try:
                trans=Utils.runCmdReturnJson(cmd)
                return trans
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    msg=ex.output.decode("utf-8")
                    Utils.Print("ERROR: Exception during get block. %s" % (msg))
                return None
        else:
            for _ in range(2):
                cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
                subcommand='db.Blocks.findOne( { "block_num": %d } )' % (blockNum)
                if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
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
                    if Utils.Debug: Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                    time.sleep(self.mongoSyncTime)

        return None

    def getBlockById(self, blockId, retry=True, silentErrors=False):
        for _ in range(2):
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand='db.Blocks.findOne( { "block_id": "%s" } )' % (blockId)
            if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
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
                if Utils.Debug: Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                time.sleep(self.mongoSyncTime)

        return None

    # def doesNodeHaveBlockNum(self, blockNum):
    #     """Does node have head_block_num >= blockNum"""
    #     assert isinstance(blockNum, int)
    #     assert (blockNum > 0)

    #     info=self.getInfo(silentErrors=True)
    #     assert(info)
    #     head_block_num=0
    #     try:
    #         head_block_num=int(info["head_block_num"])
    #     except (TypeError, KeyError) as _:
    #         Utils.Print("Failure in get info parsing. %s" % (info))
    #         raise

    #     return True if blockNum <= head_block_num else False

    def isBlockPresent(self, blockNum):
        """Does node have head_block_num >= blockNum"""
        assert isinstance(blockNum, int)
        assert (blockNum > 0)

        info=self.getInfo(silentErrors=True)
        assert(info)
        node_block_num=0
        try:
            node_block_num=int(info["head_block_num"])
        except (TypeError, KeyError) as _:
            Utils.Print("Failure in get info parsing. %s" % (info))
            raise

        return True if blockNum <= node_block_num else False

    def isBlockFinalized(self, blockNum):
        """Is blockNum finalized"""
        assert(blockNum)
        assert isinstance(blockNum, int)
        assert (blockNum > 0)

        info=self.getInfo(silentErrors=True)
        assert(info)
        node_block_num=0
        try:
            node_block_num=int(info["last_irreversible_block_num"])
        except (TypeError, KeyError) as _:
            Utils.Print("Failure in get info parsing. %s" % (info))
            raise

        return True if blockNum <= node_block_num else False

    
    # pylint: disable=too-many-branches
    def getTransaction(self, transId, retry=True, silentErrors=False):
        if not self.enableMongo:
            cmd="%s %s get transaction %s" % (Utils.EosClientPath, self.endpointArgs, transId)
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            try:
                trans=Utils.runCmdReturnJson(cmd)
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
            for _ in range(2):
                cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
                subcommand='db.Transactions.findOne( { $and : [ { "transaction_id": "%s" }, {"pending":false} ] } )' % (transId)
                if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
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
                    if Utils.Debug: Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                    time.sleep(self.mongoSyncTime)

        return None

    def isTransInBlock(self, transId, blockId):
        """Check if transId is within block identified by blockId"""
        assert(transId)
        assert(isinstance(transId, str))
        assert(blockId)
        assert(isinstance(blockId, int))

        block=self.getBlock(blockId)
        transactions=None
        try:
            transactions=block["transactions"]
        except (AssertionError, TypeError, KeyError) as _:
            Utils.Print("Failed to parse block. %s" % (block))
            raise

        if transactions is not None:
            for trans in transactions:
                assert(trans)
                try:
                    myTransId=trans["trx"]["id"]
                    if transId == myTransId:
                        return True
                except (TypeError, KeyError) as _:
                    Utils.Print("Failed to parse block transactions. %s" % (trans))

        return False

    def getBlockIdByTransId(self, transId):
        """Given a transaction Id (string), will return block id (int) containing the transaction"""
        assert(transId)
        assert(isinstance(transId, str))
        trans=self.getTransaction(transId)
        assert(trans)

        refBlockNum=None
        try:
            refBlockNum=trans["trx"]["trx"]["ref_block_num"]
            refBlockNum=int(refBlockNum)+1
        except (TypeError, ValueError, KeyError) as _:
            Utils.Print("transaction parsing failed. Transaction: %s" % (trans))
            return None

        headBlockNum=self.getHeadBlockNum()
        assert(headBlockNum)
        try:
            headBlockNum=int(headBlockNum)
        except(ValueError) as _:
            Utils.Print("Info parsing failed. %s" % (headBlockNum))

        for blockNum in range(refBlockNum, headBlockNum+1):
            if self.isTransInBlock(str(transId), blockNum):
                return blockNum

        return None

    def isTransInAnyBlock(self, transId):
        """Check if transaction (transId) is in a block."""
        assert(transId)
        assert(isinstance(transId, str))
        blockId=self.getBlockIdByTransId(transId)
        return True if blockId else False

    def isTransFinalized(self, transId):
        """Check if transaction (transId) has been finalized."""
        assert(transId)
        assert(isinstance(transId, str))
        blockId=self.getBlockIdByTransId(transId)
        if not blockId:
            return False

        assert(isinstance(blockId, int))
        return self.isBlockFinalized(blockId)

    # Disabling MongodDB funbction
    # def getTransByBlockId(self, blockId, retry=True, silentErrors=False):
    #     for _ in range(2):
    #         cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
    #         subcommand='db.Transactions.find( { "block_id": "%s" } )' % (blockId)
    #         if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
    #         try:
    #             trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand, True)
    #             if trans is not None:
    #                 return trans
    #         except subprocess.CalledProcessError as ex:
    #             if not silentErrors:
    #                 msg=ex.output.decode("utf-8")
    #                 Utils.Print("ERROR: Exception during db get trans by blockId. %s" % (msg))
    #             return None
    #         if not retry:
    #             break
    #         if self.mongoSyncTime is not None:
    #             if Utils.Debug: Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
    #             time.sleep(self.mongoSyncTime)

    #     return None

    def getActionFromDb(self, transId, retry=True, silentErrors=False):
        for _ in range(2):
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand='db.Actions.findOne( { "transaction_id": "%s" } )' % (transId)
            if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
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
                if Utils.Debug: Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                time.sleep(self.mongoSyncTime)

        return None

    def getMessageFromDb(self, transId, retry=True, silentErrors=False):
        for _ in range(2):
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand='db.Messages.findOne( { "transaction_id": "%s" } )' % (transId)
            if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
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
                if Utils.Debug: Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                time.sleep(self.mongoSyncTime)

        return None

    # Create & initialize account and return creation transactions. Return transaction json object
    def createInitializeAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False):
        cmd='%s %s system newaccount -j %s %s %s %s --stake-net "100 %s" --stake-cpu "100 %s" --buy-ram "100 %s"' % (
            Utils.EosClientPath, self.endpointArgs, creatorAccount.name, account.name,
            account.ownerPublicKey, account.activePublicKey,
            CORE_SYMBOL, CORE_SYMBOL, CORE_SYMBOL)

        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        trans=None
        try:
            trans=Utils.runCmdReturnJson(cmd)
            transId=Node.getTransId(trans)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during account creation. %s" % (msg))
            return None

        if stakedDeposit > 0:
            self.waitForTransInBlock(transId) # seems like account creation needs to be finalized before transfer can happen
            trans = self.transferFunds(creatorAccount, account, Node.currencyIntToStr(stakedDeposit, CORE_SYMBOL), "init")
            transId=Node.getTransId(trans)

        if waitForTransBlock and not self.waitForTransInBlock(transId):
            return None

        return trans

    # Create account and return creation transactions. Return transaction json object
    # waitForTransBlock: wait on creation transaction id to appear in a block
    def createAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False):
        cmd="%s %s create account -j %s %s %s %s" % (
            Utils.EosClientPath, self.endpointArgs, creatorAccount.name, account.name,
            account.ownerPublicKey, account.activePublicKey)

        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        trans=None
        try:
            trans=Utils.runCmdReturnJson(cmd)
            transId=Node.getTransId(trans)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during account creation. %s" % (msg))
            return None

        if stakedDeposit > 0:
            self.waitForTransInBlock(transId) # seems like account creation needs to be finlized before transfer can happen
            trans = self.transferFunds(creatorAccount, account, "%0.04f %s" % (stakedDeposit/10000, CORE_SYMBOL), "init")
            transId=Node.getTransId(trans)

        if waitForTransBlock and not self.waitForTransInBlock(transId):
            return None

        return trans

    def getEosAccount(self, name):
        assert(isinstance(name, str))
        cmd="%s %s get account -j %s" % (Utils.EosClientPath, self.endpointArgs, name)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Utils.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get account. %s" % (msg))
            return None

    def getEosAccountFromDb(self, name):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand='db.Accounts.findOne({"name" : "%s"})' % (name)
        if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
        try:
            trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get account from db. %s" % (msg))
            return None

    def getTable(self, contract, scope, table):
        cmd="%s %s get table %s %s %s" % (Utils.EosClientPath, self.endpointArgs, contract, scope, table)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Utils.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during table retrieval. %s" % (msg))
            return None

    def getTableAccountBalance(self, contract, scope):
        assert(isinstance(contract, str))
        assert(isinstance(scope, str))
        table="accounts"
        trans = self.getTable(contract, scope, table)
        assert(trans)
        try:
            return trans["rows"][0]["balance"]
        except (TypeError, KeyError) as _:
            print("Transaction parsing failed. Transaction: %s" % (trans))
            raise

    def getCurrencyBalance(self, contract, account, symbol=CORE_SYMBOL):
        """returns raw output from get currency balance e.g. '99999.9950 CUR'"""
        assert(contract)
        assert(isinstance(contract, str))
        assert(account)
        assert(isinstance(account, str))
        assert(symbol)
        assert(isinstance(symbol, str))
        cmd="%s %s get currency balance %s %s %s" % (Utils.EosClientPath, self.endpointArgs, contract, account, symbol)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Utils.runCmdReturnStr(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get currency stats. %s" % (msg))
            return None

    def getCurrencyStats(self, contract, symbol=CORE_SYMBOL):
        """returns Json output from get currency stats."""
        assert(contract)
        assert(isinstance(contract, str))
        assert(symbol)
        assert(isinstance(symbol, str))
        cmd="%s %s get currency stats %s %s" % (Utils.EosClientPath, self.endpointArgs, contract, symbol)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Utils.runCmdReturnJson(cmd)
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
            for _ in range(2):
                ret=self.getEosAccountFromDb(account.name)
                if ret is not None:
                    account_name=ret["name"]
                    if account_name is None:
                        Utils.Print("ERROR: Failed to verify account creation.", account.name)
                        return None
                    return ret
                if self.mongoSyncTime is not None:
                    if Utils.Debug: Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                    time.sleep(self.mongoSyncTime)

        return None

    def waitForTransInBlock(self, transId, timeout=None):
        """Wait for trans id to be finalized."""
        lam = lambda: self.isTransInAnyBlock(transId)
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForTransFinalization(self, transId, timeout=None):
        """Wait for trans id to be finalized."""
        assert(isinstance(transId, str))
        lam = lambda: self.isTransFinalized(transId)
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForNextBlock(self, timeout=None):
        num=self.getHeadBlockNum()
        lam = lambda: self.getHeadBlockNum() > num
        ret=Utils.waitForBool(lam, timeout)
        return ret

    # Trasfer funds. Returns "transfer" json return object
    def transferFunds(self, source, destination, amountStr, memo="memo", force=False, waitForTransBlock=False):
        assert isinstance(amountStr, str)
        assert(source)
        assert(isinstance(source, Account))
        assert(destination)
        assert(isinstance(destination, Account))

        cmd="%s %s -v transfer -j %s %s" % (
            Utils.EosClientPath, self.endpointArgs, source.name, destination.name)
        cmdArr=cmd.split()
        cmdArr.append(amountStr)
        cmdArr.append(memo)
        if force:
            cmdArr.append("-f")
        s=" ".join(cmdArr)
        if Utils.Debug: Utils.Print("cmd: %s" % (s))
        trans=None
        try:
            trans=Utils.runCmdArrReturnJson(cmdArr)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during funds transfer. %s" % (msg))
            return None

        assert(trans)
        transId=Node.getTransId(trans)
        if waitForTransBlock and not self.waitForTransInBlock(transId):
            return None

        return trans

    @staticmethod
    def currencyStrToInt(balanceStr):
        """Converts currency string of form "12.3456 SYS" to int 123456"""
        assert(isinstance(balanceStr, str))
        balanceStr=balanceStr.split()[0]
        #balance=int(decimal.Decimal(balanceStr[1:])*10000)
        balance=int(decimal.Decimal(balanceStr)*10000)

        return balance

    @staticmethod
    def currencyIntToStr(balance, symbol):
        """Converts currency int of form 123456 to string "12.3456 SYS" where SYS is symbol string"""
        assert(isinstance(balance, int))
        assert(isinstance(symbol, str))
        balanceStr="%.04f %s" % (balance/10000.0, symbol)

        return balanceStr

    def validateFunds(self, initialBalances, transferAmount, source, accounts):
        """Validate each account has the expected SYS balance. Validate cumulative balance matches expectedTotal."""
        assert(source)
        assert(isinstance(source, Account))
        assert(accounts)
        assert(isinstance(accounts, list))
        assert(len(accounts) > 0)
        assert(initialBalances)
        assert(isinstance(initialBalances, dict))
        assert(isinstance(transferAmount, int))

        currentBalances=self.getEosBalances([source] + accounts)
        assert(currentBalances)
        assert(isinstance(currentBalances, dict))
        assert(len(initialBalances) == len(currentBalances))

        if len(currentBalances) != len(initialBalances):
            Utils.Print("ERROR: validateFunds> accounts length mismatch. Initial: %d, current: %d" % (len(initialBalances), len(currentBalances)))
            return False

        for key, value in currentBalances.items():
            initialBalance = initialBalances[key]
            assert(initialBalances)
            expectedInitialBalance = value - transferAmount
            if key is source:
                expectedInitialBalance = value + (transferAmount*len(accounts))

            if (initialBalance != expectedInitialBalance):
                Utils.Print("ERROR: validateFunds> Expected: %d, actual: %d for account %s" %
                            (expectedInitialBalance, initialBalance, key.name))
                return False

    def getEosBalances(self, accounts):
        """Returns a dictionary with account balances keyed by accounts"""
        assert(accounts)
        assert(isinstance(accounts, list))

        balances={}
        for account in accounts:
            balance = self.getAccountEosBalance(account.name)
            balances[account]=balance

        return balances

    # Gets accounts mapped to key. Returns json object
    def getAccountsByKey(self, key):
        cmd="%s %s get accounts %s" % (Utils.EosClientPath, self.endpointArgs, key)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Utils.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during accounts by key retrieval. %s" % (msg))
            return None

    # Get actions mapped to an account (cleos get actions)
    def getActions(self, account, pos=-1, offset=-1):
        assert(isinstance(account, Account))
        assert(isinstance(pos, int))
        assert(isinstance(offset, int))

        cmd="%s %s get actions -j %s %d %d" % (Utils.EosClientPath, self.endpointArgs, account.name, pos, offset)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            actions=Utils.runCmdReturnJson(cmd)
            return actions
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during actions by account retrieval. %s" % (msg))
            return None

    # Gets accounts mapped to key. Returns array
    def getAccountsArrByKey(self, key):
        trans=self.getAccountsByKey(key)
        assert(trans)
        assert("account_names" in trans)
        accounts=trans["account_names"]
        return accounts

    def getServants(self, name):
        cmd="%s %s get servants %s" % (Utils.EosClientPath, self.endpointArgs, name)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Utils.runCmdReturnJson(cmd)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during servants retrieval. %s" % (msg))
            return None

    def getServantsArr(self, name):
        trans=self.getServants(name)
        servants=trans["controlled_accounts"]
        return servants

    def getAccountEosBalanceStr(self, scope):
        """Returns SYS currency0000 account balance from cleos get table command. Returned balance is string following syntax "98.0311 SYS". """
        assert isinstance(scope, str)
        if not self.enableMongo:
            amount=self.getTableAccountBalance("eosio.token", scope)
            if Utils.Debug: Utils.Print("getNodeAccountEosBalance %s %s" % (scope, amount))
            assert isinstance(amount, str)
            return amount
        else:
            if self.mongoSyncTime is not None:
                if Utils.Debug: Utils.Print("cmd: sleep %d seconds" % (self.mongoSyncTime))
                time.sleep(self.mongoSyncTime)

            account=self.getEosAccountFromDb(scope)
            if account is not None:
                balance=account["eos_balance"]
                return balance

        return None

    def getAccountEosBalance(self, scope):
        """Returns SYS currency0000 account balance from cleos get table command. Returned balance is an integer e.g. 980311. """
        balanceStr=self.getAccountEosBalanceStr(scope)
        balance=Node.currencyStrToInt(balanceStr)
        return balance

    def getAccountCodeHash(self, account):
        cmd="%s %s get code %s" % (Utils.EosClientPath, self.endpointArgs, account)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            retStr=Utils.checkOutput(cmd.split())
            #Utils.Print ("get code> %s"% retStr)
            p=re.compile(r'code\shash: (\w+)\n', re.MULTILINE)
            m=p.search(retStr)
            if m is None:
                msg="Failed to parse code hash."
                Utils.Print("ERROR: "+ msg)
                return None

            return m.group(1)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during code hash retrieval. %s" % (msg))
            return None

    # publish contract and return transaction as json object
    def publishContract(self, account, contractDir, wastFile, abiFile, waitForTransBlock=False, shouldFail=False):
        cmd="%s %s -v set contract -j %s %s" % (Utils.EosClientPath, self.endpointArgs, account, contractDir)
        cmd += "" if wastFile is None else (" "+ wastFile)
        cmd += "" if abiFile is None else (" " + abiFile)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        trans=None
        try:
            trans=Utils.runCmdReturnJson(cmd, trace=False)
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

        Node.validateTransaction(trans)
        transId=Node.getTransId(trans)
        if waitForTransBlock and not self.waitForTransInBlock(transId):
            return None
        return trans

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
    def pushMessage(self, account, action, data, opts, silentErrors=False):
        cmd="%s %s push action -j %s %s" % (Utils.EosClientPath, self.endpointArgs, account, action)
        cmdArr=cmd.split()
        if data is not None:
            cmdArr.append(data)
        if opts is not None:
            cmdArr += opts.split()
        s=" ".join(cmdArr)
        if Utils.Debug: Utils.Print("cmd: %s" % (s))
        try:
            trans=Utils.runCmdArrReturnJson(cmdArr)
            return (True, trans)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            if not silentErrors:
                Utils.Print("ERROR: Exception during push message. %s" % (msg))
            return (False, msg)

    def setPermission(self, account, code, pType, requirement, waitForTransBlock=False):
        cmd="%s %s set action permission -j %s %s %s %s" % (
            Utils.EosClientPath, self.endpointArgs, account, code, pType, requirement)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        trans=None
        try:
            trans=Utils.runCmdReturnJson(cmd)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during set permission. %s" % (msg))
            return None

        transId=Node.getTransId(trans)
        if waitForTransBlock and not self.waitForTransInBlock(transId):
            return None
        return trans

    def getInfo(self, silentErrors=False):
        cmd="%s %s get info" % (Utils.EosClientPath, self.endpointArgs)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            trans=Utils.runCmdReturnJson(cmd, silentErrors=silentErrors)
            return trans
        except subprocess.CalledProcessError as ex:
            if not silentErrors:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during get info. %s" % (msg))
            return None

    def getBlockFromDb(self, idx):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand="db.Blocks.find().sort({\"_id\":%d}).limit(1).pretty()" % (idx)
        if Utils.Debug: Utils.Print("cmd: echo \"%s\" | %s" % (subcommand, cmd))
        try:
            trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get db block. %s" % (msg))
            return None

    def checkPulse(self):
        info=self.getInfo(True)
        return False if info is None else True

    def getHeadBlockNum(self):
        """returns head block number(string) as returned by cleos get info."""
        if not self.enableMongo:
            info=self.getInfo()
            if info is not None:
                headBlockNumTag="head_block_num"
                return info[headBlockNumTag]
        else:
            # Either this implementation or the one in getIrreversibleBlockNum are likely wrong.
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
            # Either this implementation or the one in getHeadBlockNum are likely wrong.
            block=self.getBlockFromDb(-1)
            if block is not None:
                blockNum=block["block_num"]
                return blockNum
        return None

    def kill(self, killSignal):
        if Utils.Debug: Utils.Print("Killing node: %s" % (self.cmd))
        assert(self.pid is not None)
        try:
            os.kill(self.pid, killSignal)
        except OSError as ex:
            Utils.Print("ERROR: Failed to kill node (%d)." % (self.cmd), ex)
            return False

        # wait for kill validation
        def myFunc():
            try:
                os.kill(self.pid, 0) #check if process with pid is running
            except OSError as _:
                return True
            return False

        if not Utils.waitForBool(myFunc):
            Utils.Print("ERROR: Failed to validate node shutdown.")
            return False

        # mark node as killed
        self.pid=None
        self.killed=True
        return True

    # TBD: make nodeId an internal property
    # pylint: disable=too-many-locals
    def relaunch(self, nodeId, chainArg, newChain=False, timeout=Utils.systemWaitTimeout):

        assert(self.pid is None)
        assert(self.killed)

        if Utils.Debug: Utils.Print("Launching node process, Id: %d" % (nodeId))

        cmdArr=[]
        myCmd=self.cmd
        if not newChain:
            skip=False
            for i in self.cmd.split():
                Utils.Print("\"%s\"" % (i))
                if skip:
                    skip=False
                    continue
                if "--genesis-json" == i or "--genesis-timestamp" == i:
                    skip=True
                    continue

                cmdArr.append(i)
            myCmd=" ".join(cmdArr)

        dataDir="var/lib/node_%02d" % (nodeId)
        dt = datetime.datetime.now()
        dateStr="%d_%02d_%02d_%02d_%02d_%02d" % (
            dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)
        stdoutFile="%s/stdout.%s.txt" % (dataDir, dateStr)
        stderrFile="%s/stderr.%s.txt" % (dataDir, dateStr)
        with open(stdoutFile, 'w') as sout, open(stderrFile, 'w') as serr:
            #cmd=self.cmd + ("" if chainArg is None else (" " + chainArg))
            cmd=myCmd + ("" if chainArg is None else (" " + chainArg))
            Utils.Print("cmd: %s" % (cmd))
            popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
            self.pid=popen.pid

        def isNodeAlive():
            """wait for node to be responsive."""
            try:
                return True if self.checkPulse() else False
            except (TypeError) as _:
                pass
            return False

        isAlive=Utils.waitForBool(isNodeAlive, timeout)
        if isAlive:
            Utils.Print("Node relaunch was successfull.")
        else:
            Utils.Print("ERROR: Node relaunch Failed.")
            self.pid=None
            return False
            
        self.killed=False
        return True


###########################################################################################

Wallet=namedtuple("Wallet", "name password host port")
# pylint: disable=too-many-instance-attributes
class WalletMgr(object):
    __walletLogFile="test_keosd_output.log"
    __walletDataDir="test_wallet_0"

    # pylint: disable=too-many-arguments
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

        cmd="%s --data-dir %s --config-dir %s --http-server-address=%s:%d --verbose-http-errors --http-validate-host=false" % (
            Utils.EosWalletPath, WalletMgr.__walletDataDir, WalletMgr.__walletDataDir, self.host, self.port)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        with open(WalletMgr.__walletLogFile, 'w') as sout, open(WalletMgr.__walletLogFile, 'w') as serr:
            popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
            self.__walletPid=popen.pid

        # Give keosd time to warm up
        time.sleep(1)
        return True

    def create(self, name, accounts=None):
        wallet=self.wallets.get(name)
        if wallet is not None:
            if Utils.Debug: Utils.Print("Wallet \"%s\" already exists. Returning same." % name)
            return wallet
        p = re.compile(r'\n\"(\w+)\"\n', re.MULTILINE)
        cmd="%s %s wallet create --name %s" % (Utils.EosClientPath, self.endpointArgs, name)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        retStr=Utils.checkOutput(cmd.split())
        #Utils.Print("create: %s" % (retStr))
        m=p.search(retStr)
        if m is None:
            Utils.Print("ERROR: wallet password parser failure")
            return None
        p=m.group(1)
        wallet=Wallet(name, p, self.host, self.port)
        self.wallets[name] = wallet

        if accounts:
            for account in accounts:
                Utils.Print("Importing keys for account %s into wallet %s." % (account.name, wallet.name))
                if not self.importKey(account, wallet):
                    Utils.Print("ERROR: Failed to import key for account %s" % (account.name))
                    return False

        return wallet

    def importKey(self, account, wallet):
        warningMsg="Key already in wallet"
        cmd="%s %s wallet import --name %s %s" % (
            Utils.EosClientPath, self.endpointArgs, wallet.name, account.ownerPrivateKey)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            Utils.checkOutput(cmd.split())
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
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            try:
                Utils.checkOutput(cmd.split())
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
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            Utils.Print("ERROR: Failed to lock wallet %s." % (wallet.name))
            return False

        return True

    def unlockWallet(self, wallet):
        cmd="%s %s wallet unlock --name %s" % (Utils.EosClientPath, self.endpointArgs, wallet.name)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        popen=subprocess.Popen(cmd.split(), stdout=Utils.FNull, stdin=subprocess.PIPE)
        _, errs = popen.communicate(input=wallet.password.encode("utf-8"))
        if 0 != popen.wait():
            Utils.Print("ERROR: Failed to unlock wallet %s: %s" % (wallet.name, errs.decode("utf-8")))
            return False

        return True

    def lockAllWallets(self):
        cmd="%s %s wallet lock_all" % (Utils.EosClientPath, self.endpointArgs)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            Utils.Print("ERROR: Failed to lock all wallets.")
            return False

        return True

    def getOpenWallets(self):
        wallets=[]

        p = re.compile(r'\s+\"(\w+)\s\*\",?\n', re.MULTILINE)
        cmd="%s %s wallet list" % (Utils.EosClientPath, self.endpointArgs)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        retStr=Utils.checkOutput(cmd.split())
        #Utils.Print("retStr: %s" % (retStr))
        m=p.findall(retStr)
        if m is None:
            Utils.Print("ERROR: wallet list parser failure")
            return None
        wallets=m

        return wallets

    def getKeys(self, wallet):
        keys=[]

        p = re.compile(r'\n\s+\"(\w+)\"\n', re.MULTILINE)
        cmd="%s %s wallet private_keys --name %s --password %s " % (Utils.EosClientPath, self.endpointArgs, wallet.name, wallet.password)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        retStr=Utils.checkOutput(cmd.split())
        #Utils.Print("retStr: %s" % (retStr))
        m=p.findall(retStr)
        if m is None:
            Utils.Print("ERROR: wallet private_keys parser failure")
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

    def killall(self, allInstances=False):
        """Kill keos instances. allInstances will kill all keos instances running on the system."""
        if self.__walletPid:
            Utils.Print("Killing wallet manager process %d:" % (self.__walletPid))
            os.kill(self.__walletPid, signal.SIGKILL)

        if allInstances:
            cmd="pkill -9 %s" % (Utils.EosWalletName)
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            subprocess.call(cmd.split())


    @staticmethod
    def cleanup():
        dataDir=WalletMgr.__walletDataDir
        if os.path.isdir(dataDir) and os.path.exists(dataDir):
            shutil.rmtree(WalletMgr.__walletDataDir)



###########################################################################################
class Cluster(object):
    __chainSyncStrategies=Utils.getChainStrategies()
    __chainSyncStrategy=None
    __WalletName="MyWallet"
    __localHost="localhost"
    __BiosHost="localhost"
    __BiosPort=8788

    # pylint: disable=too-many-arguments
    # walletd [True|False] Is keosd running. If not load the wallet plugin
    def __init__(self, walletd=False, localCluster=True, host="localhost", port=8888, walletHost="localhost", walletPort=8899, enableMongo=False, mongoHost="localhost", mongoPort=27017, mongoDb="EOStest", defproduceraPrvtKey=None, defproducerbPrvtKey=None, staging=False):
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
        defproduceraPrvtKey: Defproducera account private key
        defproducerbPrvtKey: Defproducerb account private key
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
        self.defproduceraAccount=Account("defproducera")
        self.defproducerbAccount=Account("defproducerb")
        self.eosioAccount=Account("eosio")
        self.defproduceraAccount.ownerPrivateKey=defproduceraPrvtKey
        self.defproduceraAccount.activePrivateKey=defproduceraPrvtKey
        self.defproducerbAccount.ownerPrivateKey=defproducerbPrvtKey
        self.defproducerbAccount.activePrivateKey=defproducerbPrvtKey


    def setChainStrategy(self, chainSyncStrategy=Utils.SyncReplayTag):
        self.__chainSyncStrategy=self.__chainSyncStrategies.get(chainSyncStrategy)
        if self.__chainSyncStrategy is None:
            self.__chainSyncStrategy=self.__chainSyncStrategies.get("none")

    def setWalletMgr(self, walletMgr):
        self.walletMgr=walletMgr

    # launch local nodes and set self.nodes
    # pylint: disable=too-many-locals
    # pylint: disable=too-many-return-statements
    # pylint: disable=too-many-branches
    # pylint: disable=too-many-statements
    def launch(self, pnodes=1, totalNodes=1, prodCount=1, topo="mesh", p2pPlugin="net", delay=1, onlyBios=False, dontKill=False, dontBootstrap=False, totalProducers=None):
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

        if totalProducers!=None:
            producerFlag="--producers %s" % (totalProducers)
        else:
            producerFlag=""
        cmd="%s -p %s -n %s -s %s -d %s -i %s -f --p2p-plugin %s %s" % (
            Utils.EosLauncherPath, pnodes, totalNodes, topo, delay, datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
            p2pPlugin, producerFlag)
        cmdArr=cmd.split()
        if self.staging:
            cmdArr.append("--nogen")

        nodeosArgs="--max-transaction-time 5000 --abi-serializer-max-time-ms 5000 --filter-on * --p2p-max-nodes-per-host %d" % (totalNodes)
        if not self.walletd:
            nodeosArgs += " --plugin eosio::wallet_api_plugin"
        if self.enableMongo:
            nodeosArgs += " --plugin eosio::mongo_db_plugin --delete-all-blocks --mongodb-uri %s" % self.mongoUri

        if nodeosArgs:
            cmdArr.append("--nodeos")
            cmdArr.append(nodeosArgs)

        s=" ".join(cmdArr)
        if Utils.Debug: Utils.Print("cmd: %s" % (s))
        if 0 != subprocess.call(cmdArr):
            Utils.Print("ERROR: Launcher failed to launch.")
            return False

        self.nodes=list(range(totalNodes)) # placeholder for cleanup purposes only

        nodes=self.discoverLocalNodes(totalNodes, timeout=Utils.systemWaitTimeout)
        if nodes is None or totalNodes != len(nodes):
            Utils.Print("ERROR: Unable to validate %s instances, expected: %d, actual: %d" %
                          (Utils.EosServerName, totalNodes, len(nodes)))
            return False

        self.nodes=nodes

        if onlyBios:
            biosNode=Node(Cluster.__BiosHost, Cluster.__BiosPort)
            biosNode.setWalletEndpointArgs(self.walletEndpointArgs)
            if not biosNode.checkPulse():
                Utils.Print("ERROR: Bios node doesn't appear to be running...")
                return False

            self.nodes=[biosNode]

        # ensure cluster node are inter-connected by ensuring everyone has block 1
        Utils.Print("Cluster viability smoke test. Validate every cluster node has block 1. ")
        if not self.waitOnClusterBlockNumSync(1):
            Utils.Print("ERROR: Cluster doesn't seem to be in sync. Some nodes missing block 1")
            return False

        if dontBootstrap:
            Utils.Print("Skipping bootstrap.")
            return True

        Utils.Print("Bootstrap cluster.")
        if not Cluster.bootstrap(totalNodes, prodCount, Cluster.__BiosHost, Cluster.__BiosPort, dontKill, onlyBios):
            Utils.Print("ERROR: Bootstrap failed.")
            return False

        # validate iniX accounts can be retrieved

        producerKeys=Cluster.parseClusterKeys(totalNodes)
        if producerKeys is None:
            Utils.Print("ERROR: Unable to parse cluster info")
            return False

        def initAccountKeys(account, keys):
            account.ownerPrivateKey=keys["private"]
            account.ownerPublicKey=keys["public"]
            account.activePrivateKey=keys["private"]
            account.activePublicKey=keys["public"]

        initAccountKeys(self.eosioAccount, producerKeys["eosio"])
        initAccountKeys(self.defproduceraAccount, producerKeys["defproducera"])
        initAccountKeys(self.defproducerbAccount, producerKeys["defproducerb"])

        return True

    # Initialize the default nodes (at present just the root node)
    def initializeNodes(self, defproduceraPrvtKey=None, defproducerbPrvtKey=None, onlyBios=False):
        port=Cluster.__BiosPort if onlyBios else self.port
        host=Cluster.__BiosHost if onlyBios else self.host
        node=Node(host, port, enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
        node.setWalletEndpointArgs(self.walletEndpointArgs)
        if Utils.Debug: Utils.Print("Node:", node)

        node.checkPulse()
        self.nodes=[node]

        if defproduceraPrvtKey is not None:
            self.defproduceraAccount.ownerPrivateKey=defproduceraPrvtKey
            self.defproduceraAccount.activePrivateKey=defproduceraPrvtKey

        if defproducerbPrvtKey is not None:
            self.defproducerbAccount.ownerPrivateKey=defproducerbPrvtKey
            self.defproducerbAccount.activePrivateKey=defproducerbPrvtKey

        return True

    # Initialize nodes from the Json nodes string
    def initializeNodesFromJson(self, nodesJsonStr):
        nodesObj= json.loads(nodesJsonStr)
        if nodesObj is None:
            Utils.Print("ERROR: Invalid Json string.")
            return False

        if "keys" in nodesObj:
            keysMap=nodesObj["keys"]

            if "defproduceraPrivateKey" in keysMap:
                defproduceraPrivateKey=keysMap["defproduceraPrivateKey"]
                self.defproduceraAccount.ownerPrivateKey=defproduceraPrivateKey

            if "defproducerbPrivateKey" in keysMap:
                defproducerbPrivateKey=keysMap["defproducerbPrivateKey"]
                self.defproducerbAccount.ownerPrivateKey=defproducerbPrivateKey

        nArr=nodesObj["nodes"]
        nodes=[]
        for n in nArr:
            port=n["port"]
            host=n["host"]
            node=Node(host, port)
            node.setWalletEndpointArgs(self.walletEndpointArgs)
            if Utils.Debug: Utils.Print("Node:", node)

            node.checkPulse()
            nodes.append(node)

        self.nodes=nodes
        return True

    def setNodes(self, nodes):
        """manually set nodes, alternative to explicit launch"""
        self.nodes=nodes

    def waitOnClusterSync(self, timeout=None):
        """Get head block on node 0, then ensure the block is present on every cluster node."""
        assert(self.nodes)
        assert(len(self.nodes) > 0)
        targetHeadBlockNum=self.nodes[0].getHeadBlockNum() #get root nodes head block num
        if Utils.Debug: Utils.Print("Head block number on root node: %d" % (targetHeadBlockNum))
        if targetHeadBlockNum == -1:
            return False

        return self.waitOnClusterBlockNumSync(targetHeadBlockNum, timeout)

    def waitOnClusterBlockNumSync(self, targetBlockNum, timeout=None):
        """Wait for all nodes to have targetBlockNum finalized."""
        assert(self.nodes)

        def doNodesHaveBlockNum(nodes, targetBlockNum):
            for node in nodes:
                try:
                    if (not node.killed) and (not node.isBlockPresent(targetBlockNum)):
                    #if (not node.killed) and (not node.isBlockFinalized(targetBlockNum)):
                        return False
                except (TypeError) as _:
                    # This can happen if client connects before server is listening
                    return False

            return True

        lam = lambda: doNodesHaveBlockNum(self.nodes, targetBlockNum)
        ret=Utils.waitForBool(lam, timeout)
        return ret

    @staticmethod
    def createAccountKeys(count):
        accounts=[]
        p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
        for _ in range(0, count):
            try:
                cmd="%s create key" % (Utils.EosClientPath)
                if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
                keyStr=Utils.checkOutput(cmd.split())
                m=p.search(keyStr)
                if m is None:
                    Utils.Print("ERROR: Owner key creation regex mismatch")
                    break

                ownerPrivate=m.group(1)
                ownerPublic=m.group(2)

                cmd="%s create key" % (Utils.EosClientPath)
                if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
                keyStr=Utils.checkOutput(cmd.split())
                m=p.match(keyStr)
                if m is None:
                    Utils.Print("ERROR: Active key creation regex mismatch")
                    break

                activePrivate=m.group(1)
                activePublic=m.group(2)

                name=''.join(random.choice(string.ascii_lowercase) for _ in range(12))
                account=Account(name)
                account.ownerPrivateKey=ownerPrivate
                account.ownerPublicKey=ownerPublic
                account.activePrivateKey=activePrivate
                account.activePublicKey=activePublic
                accounts.append(account)
                if Utils.Debug: Utils.Print("name: %s, key(owner): ['%s', '%s], key(active): ['%s', '%s']" % (name, ownerPublic, ownerPrivate, activePublic, activePrivate))

            except subprocess.CalledProcessError as ex:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during key creation. %s" % (msg))
                break

        if count != len(accounts):
            Utils.Print("Account keys creation failed. Expected %d, actual: %d" % (count, len(accounts)))
            return None

        return accounts

    # create account keys and import into wallet. Wallet initialization will be user responsibility
    # also imports defproducera and defproducerb accounts
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

        Utils.Print("Importing keys for account %s into wallet %s." % (self.defproduceraAccount.name, wallet.name))
        if not self.walletMgr.importKey(self.defproduceraAccount, wallet):
            Utils.Print("ERROR: Failed to import key for account %s" % (self.defproduceraAccount.name))
            return False

        Utils.Print("Importing keys for account %s into wallet %s." % (self.defproducerbAccount.name, wallet.name))
        if not self.walletMgr.importKey(self.defproducerbAccount, wallet):
            Utils.Print("ERROR: Failed to import key for account %s" % (self.defproducerbAccount.name))
            return False

        for account in accounts:
            Utils.Print("Importing keys for account %s into wallet %s." % (account.name, wallet.name))
            if not self.walletMgr.importKey(account, wallet):
                Utils.Print("ERROR: Failed to import key for account %s" % (account.name))
                return False

        self.accounts=accounts
        return True

    def getNode(self, nodeId=0):
        return self.nodes[nodeId]

    def getNodes(self):
        return self.nodes

    # Spread funds across accounts with transactions spread through cluster nodes.
    #  Validate transactions are synchronized on root node
    def spreadFunds(self, source, accounts, amount=1):
        assert(source)
        assert(isinstance(source, Account))
        assert(accounts)
        assert(isinstance(accounts, list))
        assert(len(accounts) > 0)
        Utils.Print("len(accounts): %d" % (len(accounts)))

        count=len(accounts)
        transferAmount=(count*amount)+amount
        transferAmountStr=Node.currencyIntToStr(transferAmount, CORE_SYMBOL)
        node=self.nodes[0]
        fromm=source
        to=accounts[0]
        Utils.Print("Transfer %s units from account %s to %s on eos server port %d" % (
            transferAmountStr, fromm.name, to.name, node.port))
        trans=node.transferFunds(fromm, to, transferAmountStr)
        assert(trans)
        transId=Node.getTransId(trans)
        if transId is None:
            return False

        if Utils.Debug: Utils.Print("Funds transfered on transaction id %s." % (transId))

        nextEosIdx=-1
        for i in range(0, count):
            account=accounts[i]
            nextInstanceFound=False
            for _ in range(0, count):
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
            if Utils.Debug: Utils.Print("Wait for transaction id %s on node port %d" % (transId, node.port))
            if node.waitForTransInBlock(transId) is False:
                Utils.Print("ERROR: Selected node never received transaction id %s" % (transId))
                return False

            transferAmount -= amount
            transferAmountStr=Node.currencyIntToStr(transferAmount, CORE_SYMBOL)
            fromm=account
            to=accounts[i+1] if i < (count-1) else source
            Utils.Print("Transfer %s units from account %s to %s on eos server port %d." %
                    (transferAmountStr, fromm.name, to.name, node.port))

            trans=node.transferFunds(fromm, to, transferAmountStr)
            transId=Node.getTransId(trans)
            if transId is None:
                return False

            if Utils.Debug: Utils.Print("Funds transfered on block num %s." % (transId))

        # As an extra step wait for last transaction on the root node
        node=self.nodes[0]
        if Utils.Debug: Utils.Print("Wait for transaction id %s on node port %d" % (transId, node.port))
        if node.waitForTransInBlock(transId) is False:
            Utils.Print("ERROR: Selected node never received transaction id %s" % (transId))
            return False

        return True

    def validateSpreadFunds(self, initialBalances, transferAmount, source, accounts):
        """Given initial Balances, will validate each account has the expected balance based upon transferAmount.
        This validation is repeated against every node in the cluster."""
        assert(source)
        assert(isinstance(source, Account))
        assert(accounts)
        assert(isinstance(accounts, list))
        assert(len(accounts) > 0)
        assert(initialBalances)
        assert(isinstance(initialBalances, dict))
        assert(isinstance(transferAmount, int))

        for node in self.nodes:
            if node.killed:
                continue

            if Utils.Debug: Utils.Print("Validate funds on %s server port %d." %
                                        (Utils.EosServerName, node.port))

            if node.validateFunds(initialBalances, transferAmount, source, accounts) is False:
                Utils.Print("ERROR: Failed to validate funds on eos node port: %d" % (node.port))
                return False

        return True

    def spreadFundsAndValidate(self, transferAmount=1):
        """Sprays 'transferAmount' funds across configured accounts and validates action. The spray is done in a trickle down fashion with account 1
        receiving transferAmount*n SYS and forwarding x-transferAmount funds. Transfer actions are spread round-robin across the cluster to vaidate system cohesiveness."""

        if Utils.Debug: Utils.Print("Get initial system balances.")
        initialBalances=self.nodes[0].getEosBalances([self.defproduceraAccount] + self.accounts)
        assert(initialBalances)
        assert(isinstance(initialBalances, dict))

        if False == self.spreadFunds(self.defproduceraAccount, self.accounts, transferAmount):
            Utils.Print("ERROR: Failed to spread funds across nodes.")
            return False

        Utils.Print("Funds spread across all accounts. Now validate funds")

        if False == self.validateSpreadFunds(initialBalances, transferAmount, self.defproduceraAccount, self.accounts):
            Utils.Print("ERROR: Failed to validate funds transfer across nodes.")
            return False

        return True

    def validateAccounts(self, accounts, testSysAccounts=True):
        assert(len(self.nodes) > 0)
        node=self.nodes[0]

        myAccounts = []
        if testSysAccounts:
            myAccounts += [self.eosioAccount, self.defproduceraAccount, self.defproducerbAccount]
        if accounts:
            assert(isinstance(accounts, list))
            myAccounts += accounts

        node.validateAccounts(myAccounts)

    def createAccountAndVerify(self, account, creator, stakedDeposit=1000):
        """create account, verify account and return transaction id"""
        assert(len(self.nodes) > 0)
        node=self.nodes[0]
        trans=node.createInitializeAccount(account, creator, stakedDeposit)
        assert(trans)
        assert(node.verifyAccount(account))
        return trans

    # # create account, verify account and return transaction id
    # def createAccountAndVerify(self, account, creator, stakedDeposit=1000):
    #     if len(self.nodes) == 0:
    #         Utils.Print("ERROR: No nodes initialized.")
    #         return None
    #     node=self.nodes[0]

    #     transId=node.createAccount(account, creator, stakedDeposit)

    #     if transId is not None and node.verifyAccount(account) is not None:
    #         return transId
    #     return None

    def createInitializeAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False):
        assert(len(self.nodes) > 0)
        node=self.nodes[0]
        trans=node.createInitializeAccount(account, creatorAccount, stakedDeposit, waitForTransBlock)
        return trans

    @staticmethod
    def nodeNameToId(name):
        r"""Convert node name to decimal id. Node name regex is "node_([\d]+)". "node_bios" is a special name which returns -1. Examples: node_00 => 0, node_21 => 21, node_bios => -1. """
        if name == "node_bios":
            return -1

        m=re.search(r"node_([\d]+)", name)
        return int(m.group(1))


    @staticmethod
    def parseProducerKeys(configFile, nodeName):
        """Parse node config file for producer keys. Returns dictionary. (Keys: account name; Values: dictionary objects (Keys: ["name", "node", "private","public"]; Values: account name, node id returned by nodeNameToId(nodeName), private key(string)and public key(string)))."""

        configStr=None
        with open(configFile, 'r') as f:
            configStr=f.read()

        pattern=r"^\s*private-key\s*=\W+(\w+)\W+(\w+)\W+$"
        m=re.search(pattern, configStr, re.MULTILINE)
        if m is None:
            if Utils.Debug: Utils.Print("Failed to find producer keys")
            return None

        pubKey=m.group(1)
        privateKey=m.group(2)

        pattern=r"^\s*producer-name\s*=\W*(\w+)\W*$"
        matches=re.findall(pattern, configStr, re.MULTILINE)
        if matches is None:
            if Utils.Debug: Utils.Print("Failed to find producers.")
            return None

        producerKeys={}
        for m in matches:
            if Utils.Debug: Utils.Print ("Found producer : %s" % (m))
            nodeId=Cluster.nodeNameToId(nodeName)
            keys={"name": m, "node": nodeId, "private": privateKey, "public": pubKey}
            producerKeys[m]=keys

        return producerKeys

    @staticmethod
    def parseClusterKeys(totalNodes):
        """Parse cluster config file. Updates producer keys data members."""

        node="node_bios"
        configFile="etc/eosio/%s/config.ini" % (node)
        if Utils.Debug: Utils.Print("Parsing config file %s" % configFile)
        producerKeys=Cluster.parseProducerKeys(configFile, node)
        if producerKeys is None:
            Utils.Print("ERROR: Failed to parse eosio private keys from cluster config files.")
            return None

        for i in range(0, totalNodes):
            node="node_%02d" % (i)
            configFile="etc/eosio/%s/config.ini" % (node)
            if Utils.Debug: Utils.Print("Parsing config file %s" % configFile)

            keys=Cluster.parseProducerKeys(configFile, node)
            if keys is not None:
                producerKeys.update(keys)

        return producerKeys

    @staticmethod
    def bootstrap(totalNodes, prodCount, biosHost, biosPort, dontKill=False, onlyBios=False):
        """Create 'prodCount' init accounts and deposits 10000000000 SYS in each. If prodCount is -1 will initialize all possible producers.
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
        walletMgr.killall()
        walletMgr.cleanup()

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

            Node.validateTransaction(trans)

            Utils.Print("Creating accounts: %s " % ", ".join(producerKeys.keys()))
            producerKeys.pop(eosioName)
            accounts=[]
            for name, keys in producerKeys.items():
                initx = None
                initx = Account(name)
                initx.ownerPrivateKey=keys["private"]
                initx.ownerPublicKey=keys["public"]
                initx.activePrivateKey=keys["private"]
                initx.activePublicKey=keys["public"]
                trans=biosNode.createAccount(initx, eosioAccount, 0)
                if trans is None:
                    Utils.Print("ERROR: Failed to create account %s" % (name))
                    return False
                Node.validateTransaction(trans)
                accounts.append(initx)

            transId=Node.getTransId(trans)
            biosNode.waitForTransInBlock(transId)

            Utils.Print("Validating system accounts within bootstrap")
            biosNode.validateAccounts(accounts)

            if not onlyBios:
                if prodCount == -1:
                    setProdsFile="setprods.json"
                    if Utils.Debug: Utils.Print("Reading in setprods file %s." % (setProdsFile))
                    with open(setProdsFile, "r") as f:
                        setProdsStr=f.read()

                        Utils.Print("Setting producers.")
                        opts="--permission eosio@active"
                        myTrans=biosNode.pushMessage("eosio", "setprods", setProdsStr, opts)
                        if myTrans is None or not myTrans[0]:
                            Utils.Print("ERROR: Failed to set producers.")
                            return False
                else:
                    counts=dict.fromkeys(range(totalNodes), 0) #initialize node prods count to 0
                    setProdsStr='{"schedule": ['
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
                    if Utils.Debug: Utils.Print("setprods: %s" % (setProdsStr))
                    Utils.Print("Setting producers: %s." % (", ".join(prodNames)))
                    opts="--permission eosio@active"
                    # pylint: disable=redefined-variable-type
                    trans=biosNode.pushMessage("eosio", "setprods", setProdsStr, opts)
                    if trans is None or not trans[0]:
                        Utils.Print("ERROR: Failed to set producer %s." % (keys["name"]))
                        return False

                trans=trans[1]
                transId=Node.getTransId(trans)
                if not biosNode.waitForTransInBlock(transId):
                    return False

                # wait for block production handover (essentially a block produced by anyone but eosio).
                lam = lambda: biosNode.getInfo()["head_block_producer"] != "eosio"
                ret=Utils.waitForBool(lam)
                if not ret:
                    Utils.Print("ERROR: Block production handover failed.")
                    return False

            eosioTokenAccount=copy.deepcopy(eosioAccount)
            eosioTokenAccount.name="eosio.token"
            trans=biosNode.createAccount(eosioTokenAccount, eosioAccount, 0)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s" % (eosioTokenAccount.name))
                return False

            eosioRamAccount=copy.deepcopy(eosioAccount)
            eosioRamAccount.name="eosio.ram"
            trans=biosNode.createAccount(eosioRamAccount, eosioAccount, 0)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s" % (eosioRamAccount.name))
                return False

            eosioRamfeeAccount=copy.deepcopy(eosioAccount)
            eosioRamfeeAccount.name="eosio.ramfee"
            trans=biosNode.createAccount(eosioRamfeeAccount, eosioAccount, 0)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s" % (eosioRamfeeAccount.name))
                return False

            eosioStakeAccount=copy.deepcopy(eosioAccount)
            eosioStakeAccount.name="eosio.stake"
            trans=biosNode.createAccount(eosioStakeAccount, eosioAccount, 0)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s" % (eosioStakeAccount.name))
                return False

            Node.validateTransaction(trans)
            transId=Node.getTransId(trans)
            biosNode.waitForTransInBlock(transId)

            contract="eosio.token"
            contractDir="contracts/%s" % (contract)
            wastFile="contracts/%s/%s.wast" % (contract, contract)
            abiFile="contracts/%s/%s.abi" % (contract, contract)
            Utils.Print("Publish %s contract" % (contract))
            trans=biosNode.publishContract(eosioTokenAccount.name, contractDir, wastFile, abiFile, waitForTransBlock=True)
            if trans is None:
                Utils.Print("ERROR: Failed to publish contract %s." % (contract))
                return False

            # Create currency0000, followed by issue currency0000
            contract=eosioTokenAccount.name
            Utils.Print("push create action to %s contract" % (contract))
            action="create"
            data="{\"issuer\":\"%s\",\"maximum_supply\":\"1000000000.0000 %s\",\"can_freeze\":\"0\",\"can_recall\":\"0\",\"can_whitelist\":\"0\"}" % (eosioTokenAccount.name, CORE_SYMBOL)
            opts="--permission %s@active" % (contract)
            trans=biosNode.pushMessage(contract, action, data, opts)
            if trans is None or not trans[0]:
                Utils.Print("ERROR: Failed to push create action to eosio contract.")
                return False

            Node.validateTransaction(trans[1])
            transId=Node.getTransId(trans[1])
            biosNode.waitForTransInBlock(transId)

            contract=eosioTokenAccount.name
            Utils.Print("push issue action to %s contract" % (contract))
            action="issue"
            data="{\"to\":\"%s\",\"quantity\":\"1000000000.0000 %s\",\"memo\":\"initial issue\"}" % (eosioAccount.name, CORE_SYMBOL)
            opts="--permission %s@active" % (contract)
            trans=biosNode.pushMessage(contract, action, data, opts)
            if trans is None or not trans[0]:
                Utils.Print("ERROR: Failed to push issue action to eosio contract.")
                return False

            Node.validateTransaction(trans[1])
            Utils.Print("Wait for issue action transaction to become finalized.")
            transId=Node.getTransId(trans[1])
            # biosNode.waitForTransInBlock(transId)
            biosNode.waitForTransFinalization(transId)

            expectedAmount="1000000000.0000 {0}".format(CORE_SYMBOL)
            Utils.Print("Verify eosio issue, Expected: %s" % (expectedAmount))
            actualAmount=biosNode.getAccountEosBalanceStr(eosioAccount.name)
            if expectedAmount != actualAmount:
                Utils.Print("ERROR: Issue verification failed. Excepted %s, actual: %s" %
                            (expectedAmount, actualAmount))
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

            Node.validateTransaction(trans)

            initialFunds="1000000.0000 {0}".format(CORE_SYMBOL)
            Utils.Print("Transfer initial fund %s to individual accounts." % (initialFunds))
            trans=None
            contract=eosioTokenAccount.name
            action="transfer"
            for name, keys in producerKeys.items():
                data="{\"from\":\"%s\",\"to\":\"%s\",\"quantity\":\"%s\",\"memo\":\"%s\"}" % (eosioAccount.name, name, initialFunds, "init transfer")
                opts="--permission %s@active" % (eosioAccount.name)
                trans=biosNode.pushMessage(contract, action, data, opts)
                if trans is None or not trans[0]:
                    Utils.Print("ERROR: Failed to transfer funds from %s to %s." % (eosioTokenAccount.name, name))
                    return False

                Node.validateTransaction(trans[1])

            Utils.Print("Wait for last transfer transaction to become finalized.")
            transId=Node.getTransId(trans[1])
            if not biosNode.waitForTransInBlock(transId):
                return False

            Utils.Print("Cluster bootstrap done.")
        finally:
            if not dontKill:
                walletMgr.killall()
                walletMgr.cleanup()

        return True


    # Populates list of EosInstanceInfo objects, matched to actual running instances
    def discoverLocalNodes(self, totalNodes, timeout=0):
        nodes=[]

        pgrepOpts="-fl"
        # pylint: disable=deprecated-method
        if platform.linux_distribution()[0] in ["Ubuntu", "LinuxMint", "Fedora","CentOS Linux","arch"]:
            pgrepOpts="-a"

        cmd="pgrep %s %s" % (pgrepOpts, Utils.EosServerName)

        def myFunc():
            psOut=None
            try:
                if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
                psOut=Utils.checkOutput(cmd.split())
                return psOut
            except subprocess.CalledProcessError as _:
                pass
            return None

        psOut=Utils.waitForObj(myFunc, timeout)
        if psOut is None:
            Utils.Print("ERROR: No nodes discovered.")
            return nodes

        if Utils.Debug: Utils.Print("pgrep output: \"%s\"" % psOut)
        for i in range(0, totalNodes):
            pattern=r"[\n]?(\d+) (.* --data-dir var/lib/node_%02d .*)\n" % (i)
            m=re.search(pattern, psOut, re.MULTILINE)
            if m is None:
                Utils.Print("ERROR: Failed to find %s pid. Pattern %s" % (Utils.EosServerName, pattern))
                break
            instance=Node(self.host, self.port + i, pid=int(m.group(1)), cmd=m.group(2), enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
            instance.setWalletEndpointArgs(self.walletEndpointArgs)
            if Utils.Debug: Utils.Print("Node>", instance)
            nodes.append(instance)

        return nodes

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
        return True

    def relaunchEosInstances(self):

        chainArg=self.__chainSyncStrategy.arg

        newChain= False if self.__chainSyncStrategy.name in [Utils.SyncHardReplayTag, Utils.SyncNoneTag] else True
        for i in range(0, len(self.nodes)):
            node=self.nodes[i]
            if node.killed and not node.relaunch(i, chainArg, newChain=newChain):
                return False

        return True

    @staticmethod
    def dumpErrorDetailImpl(fileName):
        Utils.Print("=================================================================")
        Utils.Print("Contents of %s:" % (fileName))
        if os.path.exists(fileName):
            with open(fileName, "r") as f:
                shutil.copyfileobj(f, sys.stdout)
        else:
            Utils.Print("File %s not found." % (fileName))

    def dumpErrorDetails(self):
        fileName="etc/eosio/node_bios/config.ini"
        Cluster.dumpErrorDetailImpl(fileName)
        fileName="var/lib/node_bios/stderr.txt"
        Cluster.dumpErrorDetailImpl(fileName)

        for i in range(0, len(self.nodes)):
            fileName="etc/eosio/node_%02d/config.ini" % (i)
            Cluster.dumpErrorDetailImpl(fileName)
            fileName="var/lib/node_%02d/stderr.txt" % (i)
            Cluster.dumpErrorDetailImpl(fileName)

    def killall(self, silent=True, allInstances=False):
        """Kill cluster nodeos instances. allInstances will kill all nodeos instances running on the system."""
        cmd="%s -k 9" % (Utils.EosLauncherPath)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            if not silent: Utils.Print("Launcher failed to shut down eos cluster.")

        if allInstances:
            # ocassionally the launcher cannot kill the eos server
            cmd="pkill -9 %s" % (Utils.EosServerName)
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
                if not silent: Utils.Print("Failed to shut down eos cluster.")

        # another explicit nodes shutdown
        for node in self.nodes:
            try:
                if node.pid is not None:
                    os.kill(node.pid, signal.SIGKILL)
            except OSError as _:
                pass

    def isMongodDbRunning(self):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand="db.version()"
        if Utils.Debug: Utils.Print("echo %s | %s" % (subcommand, cmd))
        ret,outs,errs=Node.stdinAndCheckOutput(cmd.split(), subcommand)
        if ret is not 0:
            Utils.Print("ERROR: Failed to check database version: %s" % (Node.byteArrToStr(errs)) )
            return False
        if Utils.Debug: Utils.Print("MongoDb response: %s" % (outs))
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
            if Utils.Debug: Utils.Print("echo %s | %s" % (subcommand, cmd))
            ret,_,errs=Node.stdinAndCheckOutput(cmd.split(), subcommand)
            if ret is not 0:
                Utils.Print("ERROR: Failed to drop database: %s" % (Node.byteArrToStr(errs)) )


    # Create accounts and validates that the last transaction is received on root node
    def createAccounts(self, creator, waitForTransBlock=True, stakedDeposit=1000):
        if self.accounts is None:
            return True

        transId=None
        for account in self.accounts:
            if Utils.Debug: Utils.Print("Create account %s." % (account.name))
            trans=self.createAccountAndVerify(account, creator, stakedDeposit)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s." % (account.name))
                return False
            if Utils.Debug: Utils.Print("Account %s created." % (account.name))
            transId=Node.getTransId(trans)

        if waitForTransBlock and transId is not None:
            node=self.nodes[0]
            if Utils.Debug: Utils.Print("Wait for transaction id %s on server port %d." % ( transId, node.port))
            if node.waitForTransInBlock(transId) is False:
                Utils.Print("ERROR: Failed waiting for transaction id %s on server port %d." % (
                    transId, node.port))
                return False

        return True



###########################################################################################
class TestState(object):
    __LOCAL_HOST="localhost"
    __DEFAULT_PORT=8888
    parser=None
    __server=None
    __port=None
    __enableMongo=False
    __dumpErrorDetails=False
    __keepLogs=False
    __dontLaunch=False
    __dontKill=False
    __prodCount=None
    __killAll=False
    __ignoreArgs=set()
    walletMgr=None
    __testSuccessful=False
    __killEosInstances=False
    __killWallet=False
    __testSuccessful=False
    WalletdName="keosd"
    ClientName="cleos"
    cluster=None
    __args=None
    localTest=False

    def __init__(self, *ignoreArgs):
        for arg in ignoreArgs:
            self.__ignoreArgs.add(arg)
        # Override default help argument so that only --help (and not -h) can call help
        self.pnodes=1
        self.totalNodes=1
        self.totalProducers=None
        self.parser = argparse.ArgumentParser(add_help=False)
        self.parser.add_argument('-?', action='help', default=argparse.SUPPRESS,
                                 help=argparse._('show this help message and exit'))
        if "--host" not in self.__ignoreArgs:
            self.parser.add_argument("-h", "--host", type=str, help="%s host name" % (Utils.EosServerName),
                                     default=self.__LOCAL_HOST)
        if "--port" not in self.__ignoreArgs:
            self.parser.add_argument("-p", "--port", type=int, help="%s host port" % Utils.EosServerName,
                                     default=self.__DEFAULT_PORT)
        if "--prod-count" not in self.__ignoreArgs:
            self.parser.add_argument("-c", "--prod-count", type=int, help="Per node producer count", default=1)
        if "--mongodb" not in self.__ignoreArgs:
            self.parser.add_argument("--mongodb", help="Configure a MongoDb instance", action='store_true')
        if "--dump-error-details" not in self.__ignoreArgs:
            self.parser.add_argument("--dump-error-details",
                                     help="Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout",
                                     action='store_true')
        if "--dont-launch" not in self.__ignoreArgs:
            self.parser.add_argument("--dont-launch", help="Don't launch own node. Assume node is already running.",
                                     action='store_true')
        if "--keep-logs" not in self.__ignoreArgs:
            self.parser.add_argument("--keep-logs", help="Don't delete var/lib/node_* folders upon test completion",
                                     action='store_true')
        if "-v" not in self.__ignoreArgs:
            self.parser.add_argument("-v", help="verbose logging", action='store_true')
        if "--leave-running" not in self.__ignoreArgs:
            self.parser.add_argument("--leave-running", help="Leave cluster running after test finishes", action='store_true')
        if "--clean-run" not in self.__ignoreArgs:
            self.parser.add_argument("--clean-run", help="Kill all nodeos and kleos instances", action='store_true')

    def parse_args(self):
        args = self.parser.parse_args()
        if "--host" not in self.__ignoreArgs:
            self.__server=args.host
        if "--port" not in self.__ignoreArgs:
            self.__port=args.port
        if "-v" not in self.__ignoreArgs:
            Utils.Debug=args.v
        if "--mongodb" not in self.__ignoreArgs:
            self.__enableMongo=args.mongodb
        if "--dump-error-details" not in self.__ignoreArgs:
            self.__dumpErrorDetails=args.dump_error_details
        if "--keep-logs" not in self.__ignoreArgs:
            self.__keepLogs=args.keep_logs
        if "--dont-launch" not in self.__ignoreArgs:
            self.__dontLaunch=args.dont_launch
        if "--leave-running" not in self.__ignoreArgs:
            self.__dontKill=args.leave_running
        if "--prod-count" not in self.__ignoreArgs:
            self.__prodCount=args.prod_count
        if "--clean-run" not in self.__ignoreArgs:
            self.__killAll=args.clean_run
        self.walletMgr=WalletMgr(True)
        self.__testSuccessful=False
        self.__killWallet=not self.__dontKill
        self.localTest=True if self.__server == self.__LOCAL_HOST else False

    def start(self, cluster):
        Utils.Print("BEGIN")
        Utils.Print("SERVER: %s" % (self.__server))
        Utils.Print("PORT: %d" % (self.__port))

        self.walletMgr.killall(allInstances=self.__killAll)
        self.walletMgr.cleanup()
        self.cluster=cluster
        self.cluster.setWalletMgr(self.walletMgr)

        if self.localTest and not self.__dontLaunch:
            self.cluster.killall(allInstances=self.__killAll)
            self.cluster.cleanup()
            Utils.Print("Stand up cluster")
            if self.cluster.launch(prodCount=self.__prodCount, dontKill=self.__dontKill, pnodes=self.pnodes, totalNodes=self.totalNodes, totalProducers=self.totalProducers) is False:
                Utils.cmdError("launcher")
                Utils.errorExit("Failed to stand up eos cluster.")

    def end(self):
        if self.__testSuccessful:
            Utils.Print("Test succeeded.")
        else:
            Utils.Print("Test failed.")
        if not self.__testSuccessful and self.__dumpErrorDetails:
            self.cluster.dumpErrorDetails()
            self.walletMgr.dumpErrorDetails()
            Utils.Print("== Errors see above ==")

        if not self.__dontKill:
            Utils.Print("Shut down the cluster.")
            self.cluster.killall(allInstances=self.__killAll)
            if self.__testSuccessful and not self.__keepLogs:
                Utils.Print("Cleanup cluster data.")
                self.cluster.cleanup()

        if self.__killWallet:
            Utils.Print("Shut down the wallet.")
            self.walletMgr.killall(allInstances=self.__killAll)
            if self.__testSuccessful and not self.__keepLogs:
                Utils.Print("Cleanup wallet data.")
                self.walletMgr.cleanup()

    def success(self):
        self.__testSuccessful=True


###########################################################################################
