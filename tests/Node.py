import copy
import decimal
import subprocess
import time
import os
import re
import datetime
import json

from core_symbol import CORE_SYMBOL
from testUtils import Utils
from testUtils import Account
from testUtils import EnumType
from testUtils import addEnum
from testUtils import unhandledEnumType

class ReturnType(EnumType):
    pass

addEnum(ReturnType, "raw")
addEnum(ReturnType, "json")

class BlockType(EnumType):
    pass

addEnum(BlockType, "head")
addEnum(BlockType, "lib")

# pylint: disable=too-many-public-methods
class Node(object):

    # pylint: disable=too-many-instance-attributes
    # pylint: disable=too-many-arguments
    def __init__(self, host, port, pid=None, cmd=None, walletMgr=None, enableMongo=False, mongoHost="localhost", mongoPort=27017, mongoDb="EOStest"):
        self.host=host
        self.port=port
        self.pid=pid
        self.cmd=cmd
        if Utils.Debug: Utils.Print("new Node host=%s, port=%s, pid=%s, cmd=%s" % (self.host, self.port, self.pid, self.cmd))
        self.killed=False # marks node as killed
        self.enableMongo=enableMongo
        self.mongoHost=mongoHost
        self.mongoPort=mongoPort
        self.mongoDb=mongoDb
        self.endpointHttp="http://%s:%d" % (self.host, self.port)
        self.endpointArgs="--url %s" % (self.endpointHttp)
        self.mongoEndpointArgs=""
        self.infoValid=None
        self.lastRetrievedHeadBlockNum=None
        self.lastRetrievedLIB=None
        self.transCache={}
        self.walletMgr=walletMgr
        self.missingTransaction=False
        if self.enableMongo:
            self.mongoEndpointArgs += "--host %s --port %d %s" % (mongoHost, mongoPort, mongoDb)

    def eosClientArgs(self):
        walletArgs=" " + self.walletMgr.getWalletEndpointArgs() if self.walletMgr is not None else ""
        return self.endpointArgs + walletArgs + " " + Utils.MiscEosClientArgs

    def __str__(self):
        #return "Host: %s, Port:%d, Pid:%s, Cmd:\"%s\"" % (self.host, self.port, self.pid, self.cmd)
        return "Host: %s, Port:%d" % (self.host, self.port)

    @staticmethod
    def validateTransaction(trans):
        assert trans
        assert isinstance(trans, dict), print("Input type is %s" % type(trans))

        executed="executed"
        def printTrans(trans, status):
            Utils.Print("ERROR: Valid transaction should be \"%s\" but it was \"%s\"." % (executed, status))
            Utils.Print("Transaction: %s" % (json.dumps(trans, indent=1)))

        transStatus=Node.getTransStatus(trans)
        assert transStatus == executed, printTrans(trans, transStatus)

    @staticmethod
    def __printTransStructureError(trans, context):
        Utils.Print("ERROR: Failure in expected transaction structure. Missing trans%s." % (context))
        Utils.Print("Transaction: %s" % (json.dumps(trans, indent=1)))

    class Context:
        def __init__(self, obj, desc):
            self.obj=obj
            self.sections=[obj]
            self.keyContext=[]
            self.desc=desc

        def __json(self):
            return "%s=\n%s" % (self.desc, json.dumps(self.obj, indent=1))

        def __keyContext(self):
            msg=""
            for key in self.keyContext:
                if msg=="":
                    msg="["
                else:
                    msg+="]["
                msg+=key
            if msg!="":
                msg+="]"
            return msg

        def __contextDesc(self):
            return "%s%s" % (self.desc, self.__keyContext())

        def add(self, newKey):
            assert isinstance(newKey, str), print("ERROR: Trying to use %s as a key" % (newKey))
            subSection=self.sections[-1]
            assert isinstance(subSection, dict), print("ERROR: Calling \"add\" method when context is not a dictionary. %s in %s" % (self.__contextDesc(), self.__json()))
            assert newKey in subSection, print("ERROR: %s%s does not contain key \"%s\". %s" % (self.__contextDesc(), key, self.__json()))
            current=subSection[newKey]
            self.sections.append(current)
            self.keyContext.append(newKey)
            return current

        def index(self, i):
            assert isinstance(i, int), print("ERROR: Trying to use \"%s\" as a list index" % (i))
            cur=self.getCurrent()
            assert isinstance(cur, list), print("ERROR: Calling \"index\" method when context is not a list.  %s in %s" % (self.__contextDesc(), self.__json()))
            listLen=len(cur)
            assert i < listLen, print("ERROR: Index %s is beyond the size of the current list (%s).  %s in %s" % (i, listLen, self.__contextDesc(), self.__json()))
            return self.sections.append(cur[i])

        def getCurrent(self):
            return self.sections[-1]

    @staticmethod
    def getTransStatus(trans):
        cntxt=Node.Context(trans, "trans")
        cntxt.add("processed")
        cntxt.add("receipt")
        return cntxt.add("status")

    @staticmethod
    def getTransBlockNum(trans):
        cntxt=Node.Context(trans, "trans")
        cntxt.add("processed")
        cntxt.add("action_traces")
        cntxt.index(0)
        return cntxt.add("block_num")


    @staticmethod
    def stdinAndCheckOutput(cmd, subcommand):
        """Passes input to stdin, executes cmd. Returns tuple with return code(int), stdout(byte stream) and stderr(byte stream)."""
        assert(cmd)
        assert(isinstance(cmd, list))
        assert(subcommand)
        assert(isinstance(subcommand, str))
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
        tmpStr=re.sub(r'NumberLong\("(\w+)"\)', r'"NumberLong-\1"', tmpStr)
        return tmpStr

    @staticmethod
    def runMongoCmdReturnJson(cmd, subcommand, trace=False, exitOnError=False):
        """Run mongodb subcommand and return response."""
        assert(cmd)
        assert(isinstance(cmd, list))
        assert(subcommand)
        assert(isinstance(subcommand, str))
        retId,outs,errs=Node.stdinAndCheckOutput(cmd, subcommand)
        if retId is not 0:
            errorMsg="mongodb call failed. cmd=[ %s ] subcommand=\"%s\" - %s" % (", ".join(cmd), subcommand, errs)
            if exitOnError:
                Utils.cmdError(errorMsg)
                Utils.errorExit(errorMsg)

            Utils.Print("ERROR: %s" % (errMsg))
            return None
        outStr=Node.byteArrToStr(outs)
        if not outStr:
            return None
        extJStr=Utils.filterJsonObjectOrArray(outStr)
        if not extJStr:
            return None
        jStr=Node.normalizeJsonObject(extJStr)
        if not jStr:
            return None
        if trace: Utils.Print ("RAW > %s"% (outStr))
        if trace: Utils.Print ("JSON> %s"% jStr)
        try:
            jsonData=json.loads(jStr)
        except json.decoder.JSONDecodeError as _:
            Utils.Print ("ERROR: JSONDecodeError")
            Utils.Print ("Raw MongoDB response: > %s"% (outStr))
            Utils.Print ("Normalized MongoDB response: > %s"% (jStr))
            raise
        return jsonData

    @staticmethod
    def getTransId(trans):
        """Retrieve transaction id from dictionary object."""
        assert trans
        assert isinstance(trans, dict), print("Input type is %s" % type(trans))

        assert "transaction_id" in trans, print("trans does not contain key %s. trans={%s}" % ("transaction_id", json.dumps(trans, indent=2, sort_keys=True)))
        transId=trans["transaction_id"]
        return transId

    @staticmethod
    def isTrans(obj):
        """Identify if this is a transaction dictionary."""
        if obj is None or not isinstance(obj, dict):
            return False

        return True if "transaction_id" in obj else False

    @staticmethod
    def byteArrToStr(arr):
        return arr.decode("utf-8")

    def validateAccounts(self, accounts):
        assert(accounts)
        assert(isinstance(accounts, list))

        for account in accounts:
            assert(account)
            assert(isinstance(account, Account))
            if Utils.Debug: Utils.Print("Validating account %s" % (account.name))
            accountInfo=self.getEosAccount(account.name, exitOnError=True)
            try:
                if not self.enableMongo:
                    assert(accountInfo["account_name"] == account.name)
                else:
                    assert(accountInfo["name"] == account.name)
            except (AssertionError, TypeError, KeyError) as _:
                Utils.Print("account validation failed. account: %s" % (account.name))
                raise

    # pylint: disable=too-many-branches
    def getBlock(self, blockNum, silentErrors=False, exitOnError=False):
        """Given a blockId will return block details."""
        assert(isinstance(blockNum, int))
        if not self.enableMongo:
            cmdDesc="get block"
            cmd="%s %d" % (cmdDesc, blockNum)
            msg="(block number=%s)" % (blockNum);
            return self.processCleosCmd(cmd, cmdDesc, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=msg)
        else:
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand='db.blocks.findOne( { "block_num": %d } )' % (blockNum)
            if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
            start=time.perf_counter()
            try:
                block=Node.runMongoCmdReturnJson(cmd.split(), subcommand, exitOnError=exitOnError)
                if Utils.Debug:
                    end=time.perf_counter()
                    Utils.Print("cmd Duration: %.3f sec" % (end-start))

                if block is not None:
                    return block
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    end=time.perf_counter()
                    msg=ex.output.decode("utf-8")
                    errorMsg="Exception during get db node get block.  cmd Duration: %.3f sec. %s" % (end-start, msg)
                    if exitOnError:
                        Utils.cmdError(errorMsg)
                        Utils.errorExit(errorMsg)
                    else:
                        Utils.Print("ERROR: %s" % (errorMsg))
                return None

        return None

    def getBlockByIdMdb(self, blockId, silentErrors=False):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand='db.blocks.findOne( { "block_id": "%s" } )' % (blockId)
        if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
        start=time.perf_counter()
        try:
            trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
            if trans is not None:
                return trans
        except subprocess.CalledProcessError as ex:
            if not silentErrors:
                end=time.perf_counter()
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during db get block by id.  cmd Duration: %.3f sec. %s" % (end-start, msg))
            return None

        return None

    def isBlockPresent(self, blockNum, blockType=BlockType.head):
        """Does node have head_block_num/last_irreversible_block_num >= blockNum"""
        assert isinstance(blockNum, int)
        assert isinstance(blockType, BlockType)
        assert (blockNum > 0)

        info=self.getInfo(silentErrors=True, exitOnError=True)
        node_block_num=0
        try:
            if blockType==BlockType.head:
                node_block_num=int(info["head_block_num"])
            elif blockType==BlockType.lib:
                node_block_num=int(info["last_irreversible_block_num"])
            else:
                unhandledEnumType(blockType)

        except (TypeError, KeyError) as _:
            Utils.Print("Failure in get info parsing %s block. %s" % (blockType.type, info))
            raise

        present = True if blockNum <= node_block_num else False
        if Utils.Debug and blockType==BlockType.lib:
            decorator=""
            if present:
                decorator="is not "
            Utils.Print("Block %d is %sfinalized." % (blockNum, decorator))

        return present

    def isBlockFinalized(self, blockNum):
        """Is blockNum finalized"""
        return self.isBlockPresent(blockNum, blockType=BlockType.lib)

    # pylint: disable=too-many-branches
    def getTransaction(self, transId, silentErrors=False, exitOnError=False, delayedRetry=True):
        assert(isinstance(transId, str))
        exitOnErrorForDelayed=not delayedRetry and exitOnError
        timeout=3
        if not self.enableMongo:
            cmdDesc="get transaction"
            cmd="%s %s" % (cmdDesc, transId)
            msg="(transaction id=%s)" % (transId);
            for i in range(0,(int(60/timeout) - 1)):
                trans=self.processCleosCmd(cmd, cmdDesc, silentErrors=silentErrors, exitOnError=exitOnErrorForDelayed, exitMsg=msg)
                if trans is not None or not delayedRetry:
                    return trans
                if Utils.Debug: Utils.Print("Could not find transaction with id %s, delay and retry" % (transId))
                time.sleep(timeout)

            self.missingTransaction=True
            # either it is there or the transaction has timed out
            return self.processCleosCmd(cmd, cmdDesc, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=msg)
        else:
            for i in range(0,(int(60/timeout) - 1)):
                trans=self.getTransactionMdb(transId, silentErrors=silentErrors, exitOnError=exitOnErrorForDelayed)
                if trans is not None or not delayedRetry:
                    return trans
                if Utils.Debug: Utils.Print("Could not find transaction with id %s in mongodb, delay and retry" % (transId))
                time.sleep(timeout)

            return self.getTransactionMdb(transId, silentErrors=silentErrors, exitOnError=exitOnError)

    def getTransactionMdb(self, transId, silentErrors=False, exitOnError=False):
        """Get transaction from MongoDB. Since DB only contains finalized blocks, transactions can take a while to appear in DB."""
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        #subcommand='db.Transactions.findOne( { $and : [ { "trx_id": "%s" }, {"irreversible":true} ] } )' % (transId)
        subcommand='db.transactions.findOne( { "trx_id": "%s" } )' % (transId)
        if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
        start=time.perf_counter()
        try:
            trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand, exitOnError=exitOnError)
            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
            if trans is not None:
                return trans
        except subprocess.CalledProcessError as ex:
            end=time.perf_counter()
            msg=ex.output.decode("utf-8")
            errorMsg="Exception during get db node get trans in mongodb with transaction id=%s.  cmd Duration: %.3f sec.  %s" % (transId, end-start, msg)
            if exitOnError:
                Utils.cmdError("" % (errorMsg))
                Utils.errorExit("Failed to retrieve transaction in mongodb for transaction id=%s" % (transId))
            elif not silentErrors:
                Utils.Print("ERROR: %s" % (errorMsg))
            return None

    def isTransInBlock(self, transId, blockId):
        """Check if transId is within block identified by blockId"""
        assert(transId)
        assert(isinstance(transId, str))
        assert(blockId)
        assert(isinstance(blockId, int))

        block=self.getBlock(blockId, exitOnError=True)

        transactions=None
        key=""
        try:
            if not self.enableMongo:
                key="[transactions]"
                transactions=block["transactions"]
            else:
                key="[blocks][transactions]"
                transactions=block["block"]["transactions"]
        except (AssertionError, TypeError, KeyError) as _:
            Utils.Print("block%s not found. Block: %s" % (key,block))
            raise

        if transactions is not None:
            for trans in transactions:
                assert(trans)
                try:
                    myTransId=trans["trx"]["id"]
                    if transId == myTransId:
                        return True
                except (TypeError, KeyError) as _:
                    Utils.Print("transaction%s not found. Transaction: %s" % (key, trans))

        return False

    def getBlockIdByTransId(self, transId, delayedRetry=True):
        """Given a transaction Id (string), will return the actual block id (int) containing the transaction"""
        assert(transId)
        assert(isinstance(transId, str))
        trans=self.getTransaction(transId, exitOnError=True, delayedRetry=delayedRetry)

        refBlockNum=None
        key=""
        try:
            if not self.enableMongo:
                key="[trx][trx][ref_block_num]"
                refBlockNum=trans["trx"]["trx"]["ref_block_num"]
            else:
                key="[ref_block_num]"
                refBlockNum=trans["ref_block_num"]
            refBlockNum=int(refBlockNum)+1
        except (TypeError, ValueError, KeyError) as _:
            Utils.Print("transaction%s not found. Transaction: %s" % (key, trans))
            return None

        headBlockNum=self.getHeadBlockNum()
        assert(headBlockNum)
        try:
            headBlockNum=int(headBlockNum)
        except(ValueError) as _:
            Utils.Print("ERROR: Block info parsing failed. %s" % (headBlockNum))
            raise

        if Utils.Debug: Utils.Print("Reference block num %d, Head block num: %d" % (refBlockNum, headBlockNum))
        for blockNum in range(refBlockNum, headBlockNum+1):
            if self.isTransInBlock(str(transId), blockNum):
                if Utils.Debug: Utils.Print("Found transaction %s in block %d" % (transId, blockNum))
                return blockNum

        return None

    def getBlockIdByTransIdMdb(self, transId):
        """Given a transaction Id (string), will return block id (int) containing the transaction. This is specific to MongoDB."""
        assert(transId)
        assert(isinstance(transId, str))
        trans=self.getTransactionMdb(transId)
        if not trans: return None

        refBlockNum=None
        try:
            refBlockNum=trans["ref_block_num"]
            refBlockNum=int(refBlockNum)+1
        except (TypeError, ValueError, KeyError) as _:
            Utils.Print("transaction[ref_block_num] not found. Transaction: %s" % (trans))
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
        assert(isinstance(transId, (str,int)))
        # if not self.enableMongo:
        blockId=self.getBlockIdByTransId(transId)
        # else:
        #     blockId=self.getBlockIdByTransIdMdb(transId)
        return True if blockId else False

    def isTransFinalized(self, transId):
        """Check if transaction (transId) has been finalized."""
        assert(transId)
        assert(isinstance(transId, str))
        blockId=self.getBlockIdByTransId(transId)
        if not blockId:
            return False

        assert(isinstance(blockId, int))
        return self.isBlockPresent(blockId, blockType=BlockType.lib)


    # Create & initialize account and return creation transactions. Return transaction json object
    def createInitializeAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False, stakeNet=100, stakeCPU=100, buyRAM=10000, exitOnError=False):
        cmdDesc="system newaccount"
        cmd='%s -j %s %s %s %s --stake-net "%s %s" --stake-cpu "%s %s" --buy-ram "%s %s"' % (
            cmdDesc, creatorAccount.name, account.name, account.ownerPublicKey,
            account.activePublicKey, stakeNet, CORE_SYMBOL, stakeCPU, CORE_SYMBOL, buyRAM, CORE_SYMBOL)
        msg="(creator account=%s, account=%s)" % (creatorAccount.name, account.name);
        trans=self.processCleosCmd(cmd, cmdDesc, silentErrors=False, exitOnError=exitOnError, exitMsg=msg)
        self.trackCmdTransaction(trans)
        transId=Node.getTransId(trans)

        if stakedDeposit > 0:
            self.waitForTransInBlock(transId) # seems like account creation needs to be finalized before transfer can happen
            trans = self.transferFunds(creatorAccount, account, Node.currencyIntToStr(stakedDeposit, CORE_SYMBOL), "init")
            transId=Node.getTransId(trans)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def createAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False, exitOnError=False):
        """Create account and return creation transactions. Return transaction json object.
        waitForTransBlock: wait on creation transaction id to appear in a block."""
        cmdDesc="create account"
        cmd="%s -j %s %s %s %s" % (
            cmdDesc, creatorAccount.name, account.name, account.ownerPublicKey, account.activePublicKey)
        msg="(creator account=%s, account=%s)" % (creatorAccount.name, account.name);
        trans=self.processCleosCmd(cmd, cmdDesc, silentErrors=False, exitOnError=exitOnError, exitMsg=msg)
        self.trackCmdTransaction(trans)
        transId=Node.getTransId(trans)

        if stakedDeposit > 0:
            self.waitForTransInBlock(transId) # seems like account creation needs to be finlized before transfer can happen
            trans = self.transferFunds(creatorAccount, account, "%0.04f %s" % (stakedDeposit/10000, CORE_SYMBOL), "init")
            self.trackCmdTransaction(trans)
            transId=Node.getTransId(trans)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def getEosAccount(self, name, exitOnError=False):
        assert(isinstance(name, str))
        if not self.enableMongo:
            cmdDesc="get account"
            cmd="%s -j %s" % (cmdDesc, name)
            msg="( getEosAccount(name=%s) )" % (name);
            return self.processCleosCmd(cmd, cmdDesc, silentErrors=False, exitOnError=exitOnError, exitMsg=msg)
        else:
            return self.getEosAccountFromDb(name, exitOnError=exitOnError)

    def getEosAccountFromDb(self, name, exitOnError=False):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand='db.accounts.findOne({"name" : "%s"})' % (name)
        if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
        try:
            timeout = 3
            for i in range(0,(int(60/timeout) - 1)):
                start=time.perf_counter()
                trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand, exitOnError=exitOnError)
                if trans is not None:
                    if Utils.Debug:
                        end=time.perf_counter()
                        Utils.Print("cmd Duration: %.3f sec" % (end-start))
                    return trans
                time.sleep(timeout)
            return trans
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            if exitOnError:
                end=time.perf_counter()
                Utils.cmdError("Exception during get account from db for %s.  cmd Duration: %.3f sec.  %s" % (name, end-start, msg))
                Utils.errorExit("Failed during get account from db for %s. %s" % (name, msg))

            Utils.Print("ERROR: Exception during get account from db for %s. %s" % (name, msg))
            return None

    def getTable(self, contract, scope, table, exitOnError=False):
        cmdDesc = "get table"
        cmd="%s %s %s %s" % (cmdDesc, contract, scope, table)
        msg="contract=%s, scope=%s, table=%s" % (contract, scope, table);
        return self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)

    def getTableAccountBalance(self, contract, scope):
        assert(isinstance(contract, str))
        assert(isinstance(scope, str))
        table="accounts"
        trans = self.getTable(contract, scope, table, exitOnError=True)
        try:
            return trans["rows"][0]["balance"]
        except (TypeError, KeyError) as _:
            print("transaction[rows][0][balance] not found. Transaction: %s" % (trans))
            raise

    def getCurrencyBalance(self, contract, account, symbol=CORE_SYMBOL, exitOnError=False):
        """returns raw output from get currency balance e.g. '99999.9950 CUR'"""
        assert(contract)
        assert(isinstance(contract, str))
        assert(account)
        assert(isinstance(account, str))
        assert(symbol)
        assert(isinstance(symbol, str))
        cmdDesc = "get currency balance"
        cmd="%s %s %s %s" % (cmdDesc, contract, account, symbol)
        msg="contract=%s, account=%s, symbol=%s" % (contract, account, symbol);
        return self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg, returnType=ReturnType.raw)

    def getCurrencyStats(self, contract, symbol=CORE_SYMBOL, exitOnError=False):
        """returns Json output from get currency stats."""
        assert(contract)
        assert(isinstance(contract, str))
        assert(symbol)
        assert(isinstance(symbol, str))
        cmdDesc = "get currency stats"
        cmd="%s %s %s" % (cmdDesc, contract, symbol)
        msg="contract=%s, symbol=%s" % (contract, symbol);
        return self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)

    # Verifies account. Returns "get account" json return object
    def verifyAccount(self, account):
        assert(account)
        if not self.enableMongo:
            ret=self.getEosAccount(account.name)
            if ret is not None:
                account_name=ret["account_name"]
                if account_name is None:
                    Utils.Print("ERROR: Failed to verify account creation.", account.name)
                    return None
                return ret
        else:
            return self.verifyAccountMdb(account)

    def verifyAccountMdb(self, account):
        assert(account)
        ret=self.getEosAccountFromDb(account.name)
        if ret is not None:
            account_name=ret["name"]
            if account_name is None:
                Utils.Print("ERROR: Failed to verify account creation.", account.name)
                return None
            return ret

        return None

    def waitForTransInBlock(self, transId, timeout=None):
        """Wait for trans id to be finalized."""
        assert(isinstance(transId, str))
        lam = lambda: self.isTransInAnyBlock(transId)
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForTransFinalization(self, transId, timeout=None):
        """Wait for trans id to be finalized."""
        assert(isinstance(transId, str))
        lam = lambda: self.isTransFinalized(transId)
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForNextBlock(self, timeout=None, blockType=BlockType.head):
        num=self.getBlockNum(blockType=blockType)
        lam = lambda: self.getHeadBlockNum() > num
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForBlock(self, blockNum, timeout=None, blockType=BlockType.head):
        lam = lambda: self.getBlockNum(blockType=blockType) > blockNum
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForIrreversibleBlock(self, blockNum, timeout=None, blockType=BlockType.head):
        return self.waitForBlock(blockNum, timeout=timeout, blockType=blockType)

    # Trasfer funds. Returns "transfer" json return object
    def transferFunds(self, source, destination, amountStr, memo="memo", force=False, waitForTransBlock=False, exitOnError=True):
        assert isinstance(amountStr, str)
        assert(source)
        assert(isinstance(source, Account))
        assert(destination)
        assert(isinstance(destination, Account))

        cmd="%s %s -v transfer -j %s %s" % (
            Utils.EosClientPath, self.eosClientArgs(), source.name, destination.name)
        cmdArr=cmd.split()
        cmdArr.append(amountStr)
        cmdArr.append(memo)
        if force:
            cmdArr.append("-f")
        s=" ".join(cmdArr)
        if Utils.Debug: Utils.Print("cmd: %s" % (s))
        trans=None
        start=time.perf_counter()
        try:
            trans=Utils.runCmdArrReturnJson(cmdArr)
            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
            self.trackCmdTransaction(trans)
        except subprocess.CalledProcessError as ex:
            end=time.perf_counter()
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during funds transfer.  cmd Duration: %.3f sec.  %s" % (end-start, msg))
            if exitOnError:
                Utils.cmdError("could not transfer \"%s\" from %s to %s" % (amountStr, source, destination))
                Utils.errorExit("Failed to transfer \"%s\" from %s to %s" % (amountStr, source, destination))
            return None

        if trans is None:
            Utils.cmdError("could not transfer \"%s\" from %s to %s" % (amountStr, source, destination))
            Utils.errorExit("Failed to transfer \"%s\" from %s to %s" % (amountStr, source, destination))

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

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
    def getAccountsByKey(self, key, exitOnError=False):
        cmdDesc = "get accounts"
        cmd="%s %s" % (cmdDesc, key)
        msg="key=%s" % (key);
        return self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)

    # Get actions mapped to an account (cleos get actions)
    def getActions(self, account, pos=-1, offset=-1, exitOnError=False):
        assert(isinstance(account, Account))
        assert(isinstance(pos, int))
        assert(isinstance(offset, int))

        if not self.enableMongo:
            cmdDesc = "get actions"
            cmd="%s -j %s %d %d" % (cmdDesc, account.name, pos, offset)
            msg="account=%s, pos=%d, offset=%d" % (account.name, pos, offset);
            return self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)
        else:
            return self.getActionsMdb(account, pos, offset, exitOnError=exitOnError)

    def getActionsMdb(self, account, pos=-1, offset=-1, exitOnError=False):
        assert(isinstance(account, Account))
        assert(isinstance(pos, int))
        assert(isinstance(offset, int))

        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand='db.action_traces.find({$or: [{"act.data.from":"%s"},{"act.data.to":"%s"}]}).sort({"_id":%d}).limit(%d)' % (account.name, account.name, pos, abs(offset))
        if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
        start=time.perf_counter()
        try:
            actions=Node.runMongoCmdReturnJson(cmd.split(), subcommand, exitOnError=exitOnError)
            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
            if actions is not None:
                return actions
        except subprocess.CalledProcessError as ex:
            end=time.perf_counter()
            msg=ex.output.decode("utf-8")
            errorMsg="Exception during get db actions.  cmd Duration: %.3f sec.  %s" % (end-start, msg)
            if exitOnError:
                Utils.cmdError(errorMsg)
                Utils.errorExit(errorMsg)
            else:
                Utils.Print("ERROR: %s" % (errorMsg))
        return None

    # Gets accounts mapped to key. Returns array
    def getAccountsArrByKey(self, key):
        trans=self.getAccountsByKey(key)
        assert(trans)
        assert("account_names" in trans)
        accounts=trans["account_names"]
        return accounts

    def getServants(self, name, exitOnError=False):
        cmdDesc = "get servants"
        cmd="%s %s" % (cmdDesc, name)
        msg="name=%s" % (name);
        return self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)

    def getServantsArr(self, name):
        trans=self.getServants(name, exitOnError=True)
        servants=trans["controlled_accounts"]
        return servants

    def getAccountEosBalanceStr(self, scope):
        """Returns SYS currency0000 account balance from cleos get table command. Returned balance is string following syntax "98.0311 SYS". """
        assert isinstance(scope, str)
        amount=self.getTableAccountBalance("eosio.token", scope)
        if Utils.Debug: Utils.Print("getNodeAccountEosBalance %s %s" % (scope, amount))
        assert isinstance(amount, str)
        return amount

    def getAccountEosBalance(self, scope):
        """Returns SYS currency0000 account balance from cleos get table command. Returned balance is an integer e.g. 980311. """
        balanceStr=self.getAccountEosBalanceStr(scope)
        balance=Node.currencyStrToInt(balanceStr)
        return balance

    def getAccountCodeHash(self, account):
        cmd="%s %s get code %s" % (Utils.EosClientPath, self.eosClientArgs(), account)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        start=time.perf_counter()
        try:
            retStr=Utils.checkOutput(cmd.split())
            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
            #Utils.Print ("get code> %s"% retStr)
            p=re.compile(r'code\shash: (\w+)\n', re.MULTILINE)
            m=p.search(retStr)
            if m is None:
                msg="Failed to parse code hash."
                Utils.Print("ERROR: "+ msg)
                return None

            return m.group(1)
        except subprocess.CalledProcessError as ex:
            end=time.perf_counter()
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during code hash retrieval.  cmd Duration: %.3f sec.  %s" % (end-start, msg))
            return None

    # publish contract and return transaction as json object
    def publishContract(self, account, contractDir, wasmFile, abiFile, waitForTransBlock=False, shouldFail=False):
        cmd="%s %s -v set contract -j %s %s" % (Utils.EosClientPath, self.eosClientArgs(), account, contractDir)
        cmd += "" if wasmFile is None else (" "+ wasmFile)
        cmd += "" if abiFile is None else (" " + abiFile)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        trans=None
        start=time.perf_counter()
        try:
            trans=Utils.runCmdReturnJson(cmd, trace=False)
            self.trackCmdTransaction(trans)
            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
        except subprocess.CalledProcessError as ex:
            if not shouldFail:
                end=time.perf_counter()
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during code hash retrieval.  cmd Duration: %.3f sec.  %s" % (end-start, msg))
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
        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=False)

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
        cmd="%s %s push action -j %s %s" % (Utils.EosClientPath, self.eosClientArgs(), account, action)
        cmdArr=cmd.split()
        if data is not None:
            cmdArr.append(data)
        if opts is not None:
            cmdArr += opts.split()
        s=" ".join(cmdArr)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmdArr))
        start=time.perf_counter()
        try:
            trans=Utils.runCmdArrReturnJson(cmdArr)
            self.trackCmdTransaction(trans, ignoreNonTrans=True)
            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
            return (True, trans)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            if not silentErrors:
                end=time.perf_counter()
                Utils.Print("ERROR: Exception during push message.  cmd Duration=%.3f sec.  %s" % (end - start, msg))
            return (False, msg)

    def setPermission(self, account, code, pType, requirement, waitForTransBlock=False, exitOnError=False):
        cmdDesc="set action permission"
        cmd="%s -j %s %s %s %s" % (cmdDesc, account, code, pType, requirement)
        trans=self.processCleosCmd(cmd, cmdDesc, silentErrors=False, exitOnError=exitOnError)
        self.trackCmdTransaction(trans)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def delegatebw(self, fromAccount, netQuantity, cpuQuantity, toAccount=None, transferTo=False, waitForTransBlock=False, exitOnError=False):
        if toAccount is None:
            toAccount=fromAccount

        cmdDesc="system delegatebw"
        transferStr="--transfer" if transferTo else ""
        cmd="%s -j %s %s \"%s %s\" \"%s %s\" %s" % (
            cmdDesc, fromAccount.name, toAccount.name, netQuantity, CORE_SYMBOL, cpuQuantity, CORE_SYMBOL, transferStr)
        msg="fromAccount=%s, toAccount=%s" % (fromAccount.name, toAccount.name);
        trans=self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)
        self.trackCmdTransaction(trans)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def regproducer(self, producer, url, location, waitForTransBlock=False, exitOnError=False):
        cmdDesc="system regproducer"
        cmd="%s -j %s %s %s %s" % (
            cmdDesc, producer.name, producer.activePublicKey, url, location)
        msg="producer=%s" % (producer.name);
        trans=self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)
        self.trackCmdTransaction(trans)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def vote(self, account, producers, waitForTransBlock=False, exitOnError=False):
        cmdDesc = "system voteproducer prods"
        cmd="%s -j %s %s" % (
            cmdDesc, account.name, " ".join(producers))
        msg="account=%s, producers=[ %s ]" % (account.name, ", ".join(producers));
        trans=self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)
        self.trackCmdTransaction(trans)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def processCleosCmd(self, cmd, cmdDesc, silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(returnType, ReturnType))
        cmd="%s %s %s" % (Utils.EosClientPath, self.eosClientArgs(), cmd)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        if exitMsg is not None:
            exitMsg="Context: " + exitMsg
        else:
            exitMsg=""
        trans=None
        start=time.perf_counter()
        try:
            if returnType==ReturnType.json:
                trans=Utils.runCmdReturnJson(cmd, silentErrors=silentErrors)
            elif returnType==ReturnType.raw:
                trans=Utils.runCmdReturnStr(cmd)
            else:
                unhandledEnumType(returnType)

            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
        except subprocess.CalledProcessError as ex:
            if not silentErrors:
                end=time.perf_counter()
                msg=ex.output.decode("utf-8")
                errorMsg="Exception during \"%s\". Exception message: %s.  cmd Duration=%.3f sec. %s" % (cmdDesc, msg, end-start, exitMsg)
                if exitOnError:
                    Utils.cmdError(errorMsg)
                    Utils.errorExit(errorMsg)
                else:
                    Utils.Print("ERROR: %s" % (errorMsg))
            return None

        if exitOnError and trans is None:
            Utils.cmdError("could not \"%s\". %s" % (cmdDesc,exitMsg))
            errorExit("Failed to \"%s\"" % (cmdDesc))

        return trans

    def killNodeOnProducer(self, producer, whereInSequence, blockType=BlockType.head, silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(producer, str))
        assert(isinstance(whereInSequence, int))
        assert(isinstance(blockType, BlockType))
        assert(isinstance(returnType, ReturnType))
        basedOnLib="true" if blockType==BlockType.lib else "false"
        cmd="curl %s/v1/test_control/kill_node_on_producer -d '{ \"producer\":\"%s\", \"where_in_sequence\":%d, \"based_on_lib\":\"%s\" }' -X POST -H \"Content-Type: application/json\"" % \
            (self.endpointHttp, producer, whereInSequence, basedOnLib)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        rtn=None
        start=time.perf_counter()
        try:
            if returnType==ReturnType.json:
                rtn=Utils.runCmdReturnJson(cmd, silentErrors=silentErrors)
            elif returnType==ReturnType.raw:
                rtn=Utils.runCmdReturnStr(cmd)
            else:
                unhandledEnumType(returnType)

            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
        except subprocess.CalledProcessError as ex:
            if not silentErrors:
                end=time.perf_counter()
                msg=ex.output.decode("utf-8")
                errorMsg="Exception during \"%s\". %s.  cmd Duration=%.3f sec." % (cmd, msg, end-start)
                if exitOnError:
                    Utils.cmdError(errorMsg)
                    Utils.errorExit(errorMsg)
                else:
                    Utils.Print("ERROR: %s" % (errorMsg))
            return None

        if exitMsg is not None:
            exitMsg=": " + exitMsg
        else:
            exitMsg=""
        if exitOnError and rtn is None:
            Utils.cmdError("could not \"%s\" - %s" % (cmd,exitMsg))
            Utils.errorExit("Failed to \"%s\"" % (cmd))

        return rtn

    def waitForTransBlockIfNeeded(self, trans, waitForTransBlock, exitOnError=False):
        if not waitForTransBlock:
            return trans

        transId=Node.getTransId(trans)
        if not self.waitForTransInBlock(transId):
            if exitOnError:
                Utils.cmdError("transaction with id %s never made it to a block" % (transId))
                Utils.errorExit("Failed to find transaction with id %s in a block before timeout" % (transId))
            return None
        return trans

    def getInfo(self, silentErrors=False, exitOnError=False):
        cmdDesc = "get info"
        info=self.processCleosCmd(cmdDesc, cmdDesc, silentErrors=silentErrors, exitOnError=exitOnError)
        if info is None:
            self.infoValid=False
        else:
            self.infoValid=True
            self.lastRetrievedHeadBlockNum=int(info["head_block_num"])
            self.lastRetrievedLIB=int(info["last_irreversible_block_num"])
        return info

    def getBlockFromDb(self, idx):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand="db.blocks.find().sort({\"_id\":%d}).limit(1).pretty()" % (idx)
        if Utils.Debug: Utils.Print("cmd: echo \"%s\" | %s" % (subcommand, cmd))
        start=time.perf_counter()
        try:
            trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
            return trans
        except subprocess.CalledProcessError as ex:
            end=time.perf_counter()
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get db block.  cmd Duration: %.3f sec.  %s" % (end-start, msg))
            return None

    def checkPulse(self, exitOnError=False):
        info=self.getInfo(True, exitOnError=exitOnError)
        return False if info is None else True

    def getHeadBlockNum(self):
        """returns head block number(string) as returned by cleos get info."""
        if not self.enableMongo:
            info=self.getInfo(exitOnError=True)
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
            info=self.getInfo(exitOnError=True)
            if info is not None:
                return info["last_irreversible_block_num"]
        else:
            # Either this implementation or the one in getHeadBlockNum are likely wrong.
            block=self.getBlockFromDb(-1)
            if block is not None:
                blockNum=block["block_num"]
                return blockNum
        return None

    def getBlockNum(self, blockType=BlockType.head):
        assert isinstance(blockType, BlockType)
        if blockType==BlockType.head:
            return self.getHeadBlockNum()
        elif blockType==BlockType.lib:
            return self.getIrreversibleBlockNum()
        else:
            unhandledEnumType(blockType)

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

    def verifyAlive(self, silent=False):
        if not silent and Utils.Debug: Utils.Print("Checking if node(pid=%s) is alive(killed=%s): %s" % (self.pid, self.killed, self.cmd))
        if self.killed or self.pid is None:
            return False

        try:
            os.kill(self.pid, 0)
        except ProcessLookupError as ex:
            # mark node as killed
            self.pid=None
            self.killed=True
            return False
        except PermissionError as ex:
            return True
        else:
            return True

    def getBlockProducerByNum(self, blockNum, timeout=None, waitForBlock=True, exitOnError=True):
        if waitForBlock:
            self.waitForBlock(blockNum, timeout=timeout, blockType=BlockType.head)
        block=self.getBlock(blockNum, exitOnError=exitOnError)
        blockProducer=block["producer"]
        if blockProducer is None and exitOnError:
            Utils.cmdError("could not get producer for block number %s" % (blockNum))
            errorExit("Failed to get block's producer")
        return blockProducer

    def getBlockProducer(self, timeout=None, waitForBlock=True, exitOnError=True, blockType=BlockType.head):
        blockNum=self.getBlockNum(blockType=blockType)
        block=self.getBlock(blockNum, exitOnError=exitOnError, blockType=blockType)
        blockProducer=block["producer"]
        if blockProducer is None and exitOnError:
            Utils.cmdError("could not get producer for block number %s" % (blockNum))
            errorExit("Failed to get block's producer")
        return blockProducer

    def getNextCleanProductionCycle(self, trans):
        transId=Node.getTransId(trans)
        rounds=21*12*2  # max time to ensure that at least 2/3+1 of producers x blocks per producer x at least 2 times
        self.waitForTransFinalization(transId, timeout=rounds/2)
        irreversibleBlockNum=self.getIrreversibleBlockNum()

        # The voted schedule should be promoted now, then need to wait for that to become irreversible
        votingTallyWindow=120  #could be up to 120 blocks before the votes were tallied
        promotedBlockNum=self.getHeadBlockNum()+votingTallyWindow
        self.waitForIrreversibleBlock(promotedBlockNum, timeout=rounds/2)

        ibnSchedActive=self.getIrreversibleBlockNum()

        blockNum=self.getHeadBlockNum()
        Utils.Print("Searching for clean production cycle blockNum=%s ibn=%s  transId=%s  promoted bn=%s  ibn for schedule active=%s" % (blockNum,irreversibleBlockNum,transId,promotedBlockNum,ibnSchedActive))
        blockProducer=self.getBlockProducerByNum(blockNum)
        blockNum+=1
        Utils.Print("Advance until the next block producer is retrieved")
        while blockProducer == self.getBlockProducerByNum(blockNum):
            blockNum+=1

        blockProducer=self.getBlockProducerByNum(blockNum)
        return blockNum


    # TBD: make nodeId an internal property
    # pylint: disable=too-many-locals
    def relaunch(self, nodeId, chainArg, newChain=False, timeout=Utils.systemWaitTimeout, addOrSwapFlags=None):

        assert(self.pid is None)
        assert(self.killed)

        if Utils.Debug: Utils.Print("Launching node process, Id: %d" % (nodeId))

        cmdArr=[]
        myCmd=self.cmd
        toAddOrSwap=copy.deepcopy(addOrSwapFlags) if addOrSwapFlags is not None else {}
        if not newChain:
            skip=False
            swapValue=None
            for i in self.cmd.split():
                Utils.Print("\"%s\"" % (i))
                if skip:
                    skip=False
                    continue
                if "--genesis-json" == i or "--genesis-timestamp" == i:
                    skip=True
                    continue

                if swapValue is None:
                    cmdArr.append(i)
                else:
                    cmdArr.append(swapValue)
                    swapValue=None

                if i in toAddOrSwap:
                    swapValue=toAddOrSwap[i]
                    del toAddOrSwap[i]
            for k,v in toAddOrSwap.items():
                cmdArr.append(k)
                cmdArr.append(v)

            myCmd=" ".join(cmdArr)

        dataDir="var/lib/node_%02d" % (nodeId)
        dt = datetime.datetime.now()
        dateStr="%d_%02d_%02d_%02d_%02d_%02d" % (
            dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)
        stdoutFile="%s/stdout.%s.txt" % (dataDir, dateStr)
        stderrFile="%s/stderr.%s.txt" % (dataDir, dateStr)
        with open(stdoutFile, 'w') as sout, open(stderrFile, 'w') as serr:
            cmd=myCmd + ("" if chainArg is None else (" " + chainArg))
            Utils.Print("cmd: %s" % (cmd))
            popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
            self.pid=popen.pid
            if Utils.Debug: Utils.Print("restart Node host=%s, port=%s, pid=%s, cmd=%s" % (self.host, self.port, self.pid, self.cmd))

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

        self.cmd=cmd
        self.killed=False
        return True

    def trackCmdTransaction(self, trans, ignoreNonTrans=False):
        if trans is None:
            if Utils.Debug: Utils.Print("  cmd returned transaction: %s" % (trans))
            return

        if ignoreNonTrans and not Node.isTrans(trans):
            if Utils.Debug: Utils.Print("  cmd returned a non-transaction")
            return

        transId=Node.getTransId(trans)
        if Utils.Debug:
            status=Node.getTransStatus(trans)
            blockNum=Node.getTransBlockNum(trans)
            if transId in self.transCache.keys():
                replaceMsg="replacing previous trans=\n%s" % json.dumps(self.transCache[transId], indent=2, sort_keys=True)
            else:
                replaceMsg=""
            Utils.Print("  cmd returned transaction id: %s, status: %s, (possible) block num: %s %s" % (transId, status, blockNum, replaceMsg))
        self.transCache[transId]=trans

    def reportStatus(self):
        Utils.Print("Node State:")
        Utils.Print(" cmd   : %s" % (self.cmd))
        self.verifyAlive(silent=True)
        Utils.Print(" killed: %s" % (self.killed))
        Utils.Print(" host  : %s" % (self.host))
        Utils.Print(" port  : %s" % (self.port))
        Utils.Print(" pid   : %s" % (self.pid))
        status="last getInfo returned None" if not self.infoValid else "at last call to getInfo"
        Utils.Print(" hbn   : %s (%s)" % (self.lastRetrievedHeadBlockNum, status))
        Utils.Print(" lib   : %s (%s)" % (self.lastRetrievedLIB, status))
