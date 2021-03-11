import copy
import decimal
import subprocess
import time
import os
import re
import json
import signal

from datetime import datetime
from datetime import timedelta
from core_symbol import CORE_SYMBOL
from testUtils import Utils
from testUtils import Account
from testUtils import EnumType
from testUtils import addEnum
from testUtils import unhandledEnumType
from testUtils import ReturnType
from testUtils import WaitSpec

class BlockType(EnumType):
    pass

addEnum(BlockType, "head")
addEnum(BlockType, "lib")

# pylint: disable=too-many-public-methods
class Node(object):

    # pylint: disable=too-many-instance-attributes
    # pylint: disable=too-many-arguments
    def __init__(self, host, port, nodeId, pid=None, cmd=None, walletMgr=None):
        self.host=host
        self.port=port
        self.pid=pid
        self.cmd=cmd
        if nodeId != "bios":
            assert isinstance(nodeId, int)
        self.nodeId=nodeId
        if Utils.Debug: Utils.Print("new Node host=%s, port=%s, pid=%s, cmd=%s" % (self.host, self.port, self.pid, self.cmd))
        self.killed=False # marks node as killed
        self.endpointHttp="http://%s:%d" % (self.host, self.port)
        self.endpointArgs="--url %s" % (self.endpointHttp)
        self.infoValid=None
        self.lastRetrievedHeadBlockNum=None
        self.lastRetrievedLIB=None
        self.lastRetrievedHeadBlockProducer=""
        self.transCache={}
        self.walletMgr=walletMgr
        self.missingTransaction=False
        self.popenProc=None           # initial process is started by launcher, this will only be set on relaunch

    def eosClientArgs(self):
        walletArgs=" " + self.walletMgr.getWalletEndpointArgs() if self.walletMgr is not None else ""
        return self.endpointArgs + walletArgs + " " + Utils.MiscEosClientArgs

    def __str__(self):
        return "Host: %s, Port:%d, NodeNum:%s, Pid:%s" % (self.host, self.port, self.nodeId, self.pid)

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

        def hasKey(self, newKey, subSection = None):
            assert isinstance(newKey, str), print("ERROR: Trying to use %s as a key" % (newKey))
            if subSection is None:
                subSection=self.sections[-1]
            assert isinstance(subSection, dict), print("ERROR: Calling \"add\" method when context is not a dictionary. %s in %s" % (self.__contextDesc(), self.__json()))
            return newKey in subSection

        def add(self, newKey):
            subSection=self.sections[-1]
            assert self.hasKey(newKey, subSection), print("ERROR: %s does not contain key \"%s\". %s" % (self.__contextDesc(), newKey, self.__json()))
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
        # could be a transaction response
        if cntxt.hasKey("processed"):
            cntxt.add("processed")
            cntxt.add("receipt")
            return cntxt.add("status")

        # or what the history plugin returns
        cntxt.add("trx")
        cntxt.add("receipt")
        return cntxt.add("status")

    @staticmethod
    def getTransBlockNum(trans):
        cntxt=Node.Context(trans, "trans")
        # could be a transaction response
        if cntxt.hasKey("processed"):
            cntxt.add("processed")
            cntxt.add("action_traces")
            cntxt.index(0)
            return cntxt.add("block_num")

        # or what the history plugin returns
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
        tmpStr=re.sub(r'NumberLong\((\w+)\)', r'\1', tmpStr)
        return tmpStr

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
                assert(accountInfo["account_name"] == account.name)
            except (AssertionError, TypeError, KeyError) as _:
                Utils.Print("account validation failed. account: %s" % (account.name))
                raise

    # pylint: disable=too-many-branches
    def getBlock(self, blockNumOrId, silentErrors=False, exitOnError=False):
        """Given a blockId will return block details."""
        assert(isinstance(blockNumOrId, int) or isinstance(blockNumOrId, str))
        cmdDesc="get block"
        numOrId="number" if isinstance(blockNumOrId, int) else "id"
        cmd="%s %s" % (cmdDesc, blockNumOrId)
        msg="(block %s=%s)" % (numOrId, blockNumOrId)
        return self.processCleosCmd(cmd, cmdDesc, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=msg)

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
            if not present:
                decorator="not "
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
            key="[transactions]"
            transactions=block["transactions"]
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
            key="[trx][trx][ref_block_num]"
            refBlockNum=trans["trx"]["trx"]["ref_block_num"]
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

    def isTransInAnyBlock(self, transId):
        """Check if transaction (transId) is in a block."""
        assert(transId)
        assert(isinstance(transId, (str,int)))
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
        return self.isBlockPresent(blockId, blockType=BlockType.lib)


    # Create & initialize account and return creation transactions. Return transaction json object
    def createInitializeAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False, stakeNet=100, stakeCPU=100, buyRAM=10000, exitOnError=False, sign=False):
        signStr = Node.__sign_str(sign, [ creatorAccount.activePublicKey ])
        cmdDesc="system newaccount"
        cmd='%s -j %s %s \'%s\' \'%s\' --stake-net "%s %s" --stake-cpu "%s %s" --buy-ram "%s %s"' % (
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

    def createAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False, exitOnError=False, sign=False):
        """Create account and return creation transactions. Return transaction json object.
        waitForTransBlock: wait on creation transaction id to appear in a block."""
        signStr = Node.__sign_str(sign, [ creatorAccount.activePublicKey ])
        cmdDesc="create account"
        cmd="%s -j %s %s %s %s %s" % (
            cmdDesc, signStr, creatorAccount.name, account.name, account.ownerPublicKey, account.activePublicKey)
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

    def getEosAccount(self, name, exitOnError=False, returnType=ReturnType.json):
        assert(isinstance(name, str))
        cmdDesc="get account"
        jsonFlag="-j" if returnType==ReturnType.json else ""
        cmd="%s %s %s" % (cmdDesc, jsonFlag, name)
        msg="( getEosAccount(name=%s) )" % (name);
        return self.processCleosCmd(cmd, cmdDesc, silentErrors=False, exitOnError=exitOnError, exitMsg=msg, returnType=returnType)

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
        ret = self.getEosAccount(account.name)
        if ret is not None:
            account_name = ret["account_name"]
            if account_name is None:
                Utils.Print("ERROR: Failed to verify account creation.", account.name)
                return None
            return ret

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
        ret=Utils.waitForTruth(lam, timeout)
        return ret

    def waitForTransFinalization(self, transId, timeout=None):
        """Wait for trans id to be finalized."""
        assert(isinstance(transId, str))
        lam = lambda: self.isTransFinalized(transId)
        ret=Utils.waitForTruth(lam, timeout)
        return ret

    def waitForNextBlock(self, timeout=WaitSpec.default(), blockType=BlockType.head):
        num=self.getBlockNum(blockType=blockType)
        if isinstance(timeout, WaitSpec):
            timeout = timeout.seconds(num, num+1)
        lam = lambda: self.getHeadBlockNum() > num
        ret=Utils.waitForTruth(lam, timeout)
        return ret

    def waitForBlock(self, blockNum, timeout=WaitSpec.default(), blockType=BlockType.head, reportInterval=None, errorContext=None):
        currentBlockNum=self.getBlockNum(blockType=blockType)
        currentTime=time.time()
        if isinstance(timeout, WaitSpec):
            timeout.convert(currentBlockNum, blockNum)

        blockDesc = "head" if blockType == BlockType.head else "LIB"
        count = 0

        class WaitReporter:
            def __init__(self, node, reportInterval):
                self.count = 0
                self.node = node
                self.reportInterval = reportInterval

            def __call__(self):
                self.count += 1
                if self.count % self.reportInterval == 0:
                    info = self.node.getInfo()
                    Utils.Print("Waiting on %s block num %d, get info = {\n%s\n}" % (blockDesc, blockNum, info))

        class RequireBlockNum:
            def __init__(self, node, blockNum):
                self.node = node
                self.blockNum = blockNum
                self.lastBlockNum = None
                self.passed = False
                self.advanced = None

            def __call__(self):
                currentBlockNum = self.node.getBlockNum(blockType=blockType)
                self.advanced = False
                if self.lastBlockNum is None or self.lastBlockNum < currentBlockNum:
                    self.advanced = True
                elif self.lastBlockNum > currentBlockNum:
                    Utils.Print("waitForBlock is waiting to reach block number: %d and the block number has rolled back from %d to %d." %
                                (self.blockNum, self.lastBlockNum, currentBlockNum))
                self.lastBlockNum = currentBlockNum
                self.passed = self.lastBlockNum > self.blockNum
                return self.passed

            def __enter__(self):
                return self

            def __exit__(self, exc_type, exc_value, exc_traceback):
                if not self.passed:
                    notAdvanceStr="(but has not changed since last sleep)" if not self.advanced else ""
                    Utils.Print("waitForBlock never reached block number: %d.  It started at: %d and had progressed%s to: %d after %d seconds." %
                                (blockNum, currentBlockNum, notAdvanceStr, self.lastBlockNum, time.time()-currentTime))

        with RequireBlockNum(self, blockNum) as lam:

            reporter = WaitReporter(self, reportInterval) if reportInterval is not None else None
            ret=Utils.waitForTruth(lam, timeout, reporter=reporter)

        assert ret is not None or errorContext is None, Utils.errorExit("%s." % (errorContext))
        return ret

    def waitForIrreversibleBlock(self, blockNum, timeout=WaitSpec.default(), blockType=BlockType.head):
        return self.waitForBlock(blockNum, timeout=timeout, blockType=blockType)

    # Trasfer funds. Returns "transfer" json return object
    def transferFunds(self, source, destination, amountStr, memo="memo", force=False, waitForTransBlock=False, exitOnError=True, reportStatus=True, sign=False, dontSend=False, expiration=None, skipSign=False):
        assert isinstance(amountStr, str)
        assert(source)
        assert(isinstance(source, Account))
        assert(destination)
        assert(isinstance(destination, Account))
        assert(expiration is None or isinstance(expiration, int))

        dontSendStr = ""
        if dontSend:
            dontSendStr = "--dont-broadcast "
            if expiration is None:
                # default transaction expiration to be 4 minutes in the future
                expiration = 240

        expirationStr = ""
        if expiration is not None:
            expirationStr = "--expiration %d " % (expiration)
        cmd="%s %s -v transfer -j %s %s" % (
            Utils.EosClientPath, self.eosClientArgs(), dontSendStr, expirationStr)
        cmdArr=cmd.split()
        # not using __sign_str, since cmdArr messes up the string
        if sign:
            cmdArr.append("--sign-with")
            cmdArr.append("[ \"%s\" ]" % (source.activePublicKey))

        if skipSign:
            cmdArr.append("--skip-sign")

        cmdArr.append(source.name)
        cmdArr.append(destination.name)
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
            if not dontSend:
                self.trackCmdTransaction(trans, reportStatus=reportStatus)
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

        cmdDesc = "get actions"
        cmd = "%s -j %s %d %d" % (cmdDesc, account.name, pos, offset)
        msg = "account=%s, pos=%d, offset=%d" % (account.name, pos, offset);
        return self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)

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
    def publishContract(self, account, contractDir, wasmFile, abiFile, waitForTransBlock=False, shouldFail=False, sign=False):
        signStr = Node.__sign_str(sign, [ account.activePublicKey ])
        cmd="%s %s -v set contract -j %s %s %s" % (Utils.EosClientPath, self.eosClientArgs(), signStr, account.name, contractDir)
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
                Utils.Print("ERROR: Exception during set contract.  cmd Duration: %.3f sec.  %s" % (end-start, msg))
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

    # returns tuple with indication if transaction was successfully sent and either the transaction or else the exception output
    def pushMessage(self, account, action, data, opts, silentErrors=False, signatures=None):
        cmd="%s %s push action -j %s %s" % (Utils.EosClientPath, self.eosClientArgs(), account, action)
        cmdArr=cmd.split()
        # not using __sign_str, since cmdArr messes up the string
        if signatures is not None:
            cmdArr.append("--sign-with")
            cmdArr.append("[ \"%s\" ]" % ("\", \"".join(signatures)))
        if data is not None:
            cmdArr.append(data)
        if opts is not None:
            cmdArr += opts.split()
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

    # returns tuple with indication if transaction was successfully sent and either the transaction or else the exception output
    def pushTransaction(self, trans, opts="--skip-sign", silentErrors=False, permissions=None):
        assert(isinstance(trans, dict))
        if isinstance(permissions, str):
            permissions=[permissions]
        cmd="%s %s push transaction -j" % (Utils.EosClientPath, self.eosClientArgs())
        cmdArr=cmd.split()
        transStr = json.dumps(trans, separators=(',', ':'))
        transStr = transStr.replace("'", '"')
        cmdArr.append(transStr)
        if opts is not None:
            cmdArr += opts.split()
        if permissions is not None:
            for permission in permissions:
                cmdArr.append("-p")
                cmdArr.append(permission)

        s=" ".join(cmdArr)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmdArr))
        start=time.perf_counter()
        try:
            retTrans=Utils.runCmdArrReturnJson(cmdArr)
            self.trackCmdTransaction(retTrans, ignoreNonTrans=True)
            if Utils.Debug:
                end=time.perf_counter()
                Utils.Print("cmd Duration: %.3f sec" % (end-start))
            return (True, retTrans)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            if not silentErrors:
                end=time.perf_counter()
                Utils.Print("ERROR: Exception during push message.  cmd Duration=%.3f sec.  %s" % (end - start, msg))
            return (False, msg)

    @staticmethod
    def __sign_str(sign, keys):
        assert(isinstance(sign, bool))
        assert(isinstance(keys, list))
        if not sign:
            return ""

        return "--sign-with '[ \"" + "\", \"".join(keys) + "\" ]'"

    def setPermission(self, account, code, pType, requirement, waitForTransBlock=False, exitOnError=False, sign=False):
        assert(isinstance(account, Account))
        assert(isinstance(code, Account))
        signStr = Node.__sign_str(sign, [ account.activePublicKey ])
        cmdDesc="set action permission"
        cmd="%s -j %s %s %s %s %s" % (cmdDesc, signStr, account.name, code.name, pType, requirement)
        trans=self.processCleosCmd(cmd, cmdDesc, silentErrors=False, exitOnError=exitOnError)
        self.trackCmdTransaction(trans)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def delegatebw(self, fromAccount, netQuantity, cpuQuantity, toAccount=None, transferTo=False, waitForTransBlock=False, exitOnError=False, sign=False, reportStatus=True):
        if toAccount is None:
            toAccount=fromAccount

        signStr = Node.__sign_str(sign, [ fromAccount.activePublicKey ])
        cmdDesc="system delegatebw"
        transferStr="--transfer" if transferTo else ""
        cmd="%s -j %s %s %s \"%s %s\" \"%s %s\" %s" % (
            cmdDesc, signStr, fromAccount.name, toAccount.name, netQuantity, CORE_SYMBOL, cpuQuantity, CORE_SYMBOL, transferStr)
        msg="fromAccount=%s, toAccount=%s" % (fromAccount.name, toAccount.name);
        trans=self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)
        self.trackCmdTransaction(trans, reportStatus=reportStatus)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def undelegatebw(self, fromAccount, netQuantity, cpuQuantity, toAccount=None, waitForTransBlock=False, exitOnError=False, sign=False):
        if toAccount is None:
            toAccount=fromAccount

        signStr = Node.__sign_str(sign, [ fromAccount.activePublicKey ])
        cmdDesc="system undelegatebw"
        cmd="%s -j %s %s %s \"%s %s\" \"%s %s\"" % (
            cmdDesc, signStr, fromAccount.name, toAccount.name, netQuantity, CORE_SYMBOL, cpuQuantity, CORE_SYMBOL)
        msg="fromAccount=%s, toAccount=%s" % (fromAccount.name, toAccount.name);
        trans=self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)
        self.trackCmdTransaction(trans)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def regproducer(self, producer, url, location, waitForTransBlock=False, exitOnError=False, sign=False):
        signStr = Node.__sign_str(sign, [ producer.activePublicKey ])
        cmdDesc="system regproducer"
        cmd="%s -j %s %s %s %s %s" % (
            cmdDesc, signStr, producer.name, producer.activePublicKey, url, location)
        msg="producer=%s" % (producer.name);
        trans=self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)
        self.trackCmdTransaction(trans)

        return self.waitForTransBlockIfNeeded(trans, waitForTransBlock, exitOnError=exitOnError)

    def vote(self, account, producers, waitForTransBlock=False, exitOnError=False, sign=False):
        signStr = Node.__sign_str(sign, [ account.activePublicKey ])
        cmdDesc = "system voteproducer prods"
        cmd="%s -j %s %s %s" % (
            cmdDesc, signStr, account.name, " ".join(producers))
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
            Utils.errorExit("Failed to \"%s\"" % (cmdDesc))

        return trans

    def killNodeOnProducer(self, producer, whereInSequence, blockType=BlockType.head, silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(producer, str))
        assert(isinstance(whereInSequence, int))
        assert(isinstance(blockType, BlockType))
        assert(isinstance(returnType, ReturnType))
        basedOnLib="true" if blockType==BlockType.lib else "false"
        payload="{ \"producer\":\"%s\", \"where_in_sequence\":%d, \"based_on_lib\":\"%s\" }" % (producer, whereInSequence, basedOnLib)
        return self.processCurlCmd("test_control", "kill_node_on_producer", payload, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=exitMsg, returnType=returnType)

    def processCurlCmd(self, resource, command, payload, silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        cmd="curl %s/v1/%s/%s -d '%s' -X POST -H \"Content-Type: application/json\"" % \
            (self.endpointHttp, resource, command, payload)
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
                printReturn=json.dumps(rtn) if returnType==ReturnType.json else rtn
                Utils.Print("cmd returned: %s" % (printReturn))
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

    def txnGenCreateTestAccounts(self, genAccount, genKey, silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(genAccount, str))
        assert(isinstance(genKey, str))
        assert(isinstance(returnType, ReturnType))

        payload="[ \"%s\", \"%s\" ]" % (genAccount, genKey)
        return self.processCurlCmd("txn_test_gen", "create_test_accounts", payload, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=exitMsg, returnType=returnType)

    def txnGenStart(self, salt, period, batchSize, silentErrors=True, exitOnError=False, exitMsg=None, returnType=ReturnType.json):
        assert(isinstance(salt, str))
        assert(isinstance(period, int))
        assert(isinstance(batchSize, int))
        assert(isinstance(returnType, ReturnType))

        payload="[ \"%s\", %d, %d ]" % (salt, period, batchSize)
        return self.processCurlCmd("txn_test_gen", "start_generation", payload, silentErrors=silentErrors, exitOnError=exitOnError, exitMsg=exitMsg, returnType=returnType)

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
            self.lastRetrievedHeadBlockProducer=info["head_block_producer"]
        return info

    def checkPulse(self, exitOnError=False):
        info=self.getInfo(True, exitOnError=exitOnError)
        return False if info is None else True

    def getHeadBlockNum(self):
        """returns head block number(string) as returned by cleos get info."""
        info = self.getInfo(exitOnError=True)
        if info is not None:
            headBlockNumTag = "head_block_num"
            return info[headBlockNumTag]

    def getIrreversibleBlockNum(self):
        info = self.getInfo(exitOnError=True)
        if info is not None:
            Utils.Print("current lib: %d" % (info["last_irreversible_block_num"]))
            return info["last_irreversible_block_num"]

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
        assert (self.pid is not None)
        Utils.Print("Killing node pid: {}", self.pid)
        try:
            if self.popenProc is not None:
               Utils.Print("self.popenProc is not None")
               self.popenProc.send_signal(killSignal)
               self.popenProc.wait()
            else:
               os.kill(self.pid, killSignal)
        except OSError as ex:
            Utils.Print("ERROR: Failed to kill node (%s)." % (self.cmd), ex)
            return False

        # wait for kill validation
        def myFunc():
            try:
                os.kill(self.pid, 0) #check if process with pid is running
            except OSError as _:
                return True
            return False

        if not Utils.waitForTruth(myFunc):
            Utils.Print("ERROR: Failed to validate node shutdown.")
            return False

        # mark node as killed
        self.pid=None
        self.killed=True
        return True

    def interruptAndVerifyExitStatus(self, timeout=60):
        if Utils.Debug: Utils.Print("terminating node: %s" % (self.cmd))
        assert self.popenProc is not None, "node: \"%s\" does not have a popenProc, this may be because it is only set after a relaunch." % (self.cmd)
        self.popenProc.send_signal(signal.SIGINT)
        try:
            outs, _ = self.popenProc.communicate(timeout=timeout)
            assert self.popenProc.returncode == 0, "Expected terminating \"%s\" to have an exit status of 0, but got %d" % (self.cmd, self.popenProc.returncode)
        except subprocess.TimeoutExpired:
            Utils.errorExit("Terminate call failed on node: %s" % (self.cmd))

        # mark node as killed
        self.pid=None
        self.killed=True

    def verifyAlive(self, silent=False):
        logStatus=not silent and Utils.Debug
        pid=self.pid
        if logStatus: Utils.Print("Checking if node(pid=%s) is alive(killed=%s): %s" % (self.pid, self.killed, self.cmd))
        if self.killed or self.pid is None:
            self.killed=True
            self.pid=None
            return False

        try:
            os.kill(self.pid, 0)
        except ProcessLookupError as ex:
            # mark node as killed
            self.pid=None
            self.killed=True
            if logStatus: Utils.Print("Determined node(formerly pid=%s) is killed" % (pid))
            return False
        except PermissionError as ex:
            if logStatus: Utils.Print("Determined node(formerly pid=%s) is alive" % (pid))
            return True

        if logStatus: Utils.Print("Determined node(pid=%s) is alive" % (self.pid))
        return True

    @staticmethod
    def getBlockAttribute(block, key, blockNum, exitOnError=True):
        value=block[key]

        if value is None and exitOnError:
            blockNumStr=" for block number %s" % (blockNum)
            blockStr=" with block content:\n%s" % (json.dumps(block, indent=4, sort_keys=True))
            Utils.cmdError("could not get %s%s%s" % (key, blockNumStr, blockStr))
            Utils.errorExit("Failed to get block's %s" % (key))

        return value

    def getBlockProducerByNum(self, blockNum, timeout=None, waitForBlock=True, exitOnError=True):
        if waitForBlock:
            self.waitForBlock(blockNum, timeout=timeout, blockType=BlockType.head)
        block=self.getBlock(blockNum, exitOnError=exitOnError)
        return Node.getBlockAttribute(block, "producer", blockNum, exitOnError=exitOnError)

    def getBlockProducer(self, timeout=None, waitForBlock=True, exitOnError=True, blockType=BlockType.head):
        blockNum=self.getBlockNum(blockType=blockType)
        block=self.getBlock(blockNum, exitOnError=exitOnError, blockType=blockType)
        return Node.getBlockAttribute(block, "producer", blockNum, exitOnError=exitOnError)

    def getNextCleanProductionCycle(self, trans):
        rounds=21*12*2  # max time to ensure that at least 2/3+1 of producers x blocks per producer x at least 2 times
        if trans is not None:
            transId=Node.getTransId(trans)
            self.waitForTransFinalization(transId, timeout=rounds/2)
        else:
            transId="Null"
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


    # pylint: disable=too-many-locals
    # If nodeosPath is equal to None, it will use the existing nodeos path
    def relaunch(self, chainArg=None, newChain=False, skipGenesis=True, timeout=Utils.systemWaitTimeout, addSwapFlags=None, cachePopen=False, nodeosPath=None):

        assert(self.pid is None)
        assert(self.killed)

        if Utils.Debug: Utils.Print("Launching node process, Id: {}".format(self.nodeId))

        cmdArr=[]
        splittedCmd=self.cmd.split()
        if nodeosPath: splittedCmd[0] = nodeosPath
        myCmd=" ".join(splittedCmd)
        toAddOrSwap=copy.deepcopy(addSwapFlags) if addSwapFlags is not None else {}
        if not newChain:
            skip=False
            swapValue=None
            for i in splittedCmd:
                Utils.Print("\"%s\"" % (i))
                if skip:
                    skip=False
                    continue
                if skipGenesis and ("--genesis-json" == i or "--genesis-timestamp" == i):
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

        cmd=myCmd + ("" if chainArg is None else (" " + chainArg))
        self.launchCmd(cmd, cachePopen)

        def isNodeAlive():
            """wait for node to be responsive."""
            try:
                return True if self.checkPulse() else False
            except (TypeError) as _:
                pass
            return False

        class DidProcessExitGracefully:
            def __init__(self, popen, timeout):
                self.popen = popen
                self.timeout = timeout

            def __call__(self):
                try:
                    self.popen.communicate(timeout=self.timeout)
                except TimeoutExpired:
                    return False
                with open(self.popen.errfile.name, 'r') as f:
                    if "Reached configured maximum block 10; terminating" in f.read():
                        return True
                    else:
                        return False

        if "terminate-at-block" not in cmd:
            isAlive=Utils.waitForTruth(isNodeAlive, timeout, sleepTime=1)
        else:
            lam=DidProcessExitGracefully(self.popenProc, timeout)
            isAlive=Utils.waitForTruth(lam, timeout, sleepTime=1)
        if isAlive:
            Utils.Print("Node relaunch was successful.")
        else:
            Utils.Print("ERROR: Node relaunch Failed.")
            # Ensure the node process is really killed
            if self.popenProc:
                self.popenProc.send_signal(signal.SIGTERM)
                self.popenProc.wait()
            self.pid=None
            return False

        self.cmd=cmd
        self.killed=False
        return True

    @staticmethod
    def unstartedFile(nodeId):
        assert(isinstance(nodeId, int))
        startFile=Utils.getNodeDataDir(nodeId, "start.cmd")
        if not os.path.exists(startFile):
            Utils.errorExit("Cannot find unstarted node since %s file does not exist" % startFile)
        return startFile

    def launchUnstarted(self, cachePopen=False):
        Utils.Print("launchUnstarted cmd: %s" % (self.cmd))
        self.launchCmd(self.cmd, cachePopen)

    def launchCmd(self, cmd, cachePopen=False):
        dataDir=Utils.getNodeDataDir(self.nodeId)
        dt = datetime.now()
        dateStr=Utils.getDateString(dt)
        stdoutFile="%s/stdout.%s.txt" % (dataDir, dateStr)
        stderrFile="%s/stderr.%s.txt" % (dataDir, dateStr)
        with open(stdoutFile, 'w') as sout, open(stderrFile, 'w') as serr:
            Utils.Print("cmd: %s" % (cmd))
            popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
            if cachePopen:
                popen.outfile=sout
                popen.errfile=serr
                self.popenProc=popen
            self.pid=popen.pid
            if Utils.Debug: Utils.Print("start Node host=%s, port=%s, pid=%s, cmd=%s" % (self.host, self.port, self.pid, self.cmd))

    def trackCmdTransaction(self, trans, ignoreNonTrans=False, reportStatus=True):
        if trans is None:
            if Utils.Debug: Utils.Print("  cmd returned transaction: %s" % (trans))
            return

        if ignoreNonTrans and not Node.isTrans(trans):
            if Utils.Debug: Utils.Print("  cmd returned a non-transaction: %s" % (trans))
            return

        transId=Node.getTransId(trans)
        if transId in self.transCache.keys():
            replaceMsg="replacing previous trans=\n%s" % json.dumps(self.transCache[transId], indent=2, sort_keys=True)
        else:
            replaceMsg=""

        if Utils.Debug and reportStatus:
            status=Node.getTransStatus(trans)
            blockNum=Node.getTransBlockNum(trans)
            Utils.Print("  cmd returned transaction id: %s, status: %s, (possible) block num: %s %s" % (transId, status, blockNum, replaceMsg))
        elif Utils.Debug:
            Utils.Print("  cmd returned transaction id: %s %s" % (transId, replaceMsg))

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

    # Require producer_api_plugin
    def scheduleProtocolFeatureActivations(self, featureDigests=[]):
        param = { "protocol_features_to_activate": featureDigests }
        self.processCurlCmd("producer", "schedule_protocol_feature_activations", json.dumps(param))

    # Require producer_api_plugin
    def getSupportedProtocolFeatures(self, excludeDisabled=False, excludeUnactivatable=False):
        param = {
           "exclude_disabled": excludeDisabled,
           "exclude_unactivatable": excludeUnactivatable
        }
        res = self.processCurlCmd("producer", "get_supported_protocol_features", json.dumps(param))
        return res

    # This will return supported protocol features in a dict (feature codename as the key), i.e.
    # {
    #   "PREACTIVATE_FEATURE": {...},
    #   "ONLY_LINK_TO_EXISTING_PERMISSION": {...},
    # }
    # Require producer_api_plugin
    def getSupportedProtocolFeatureDict(self, excludeDisabled=False, excludeUnactivatable=False):
        protocolFeatureDigestDict = {}
        supportedProtocolFeatures = self.getSupportedProtocolFeatures(excludeDisabled, excludeUnactivatable)
        for protocolFeature in supportedProtocolFeatures:
            for spec in protocolFeature["specification"]:
                if (spec["name"] == "builtin_feature_codename"):
                    codename = spec["value"]
                    protocolFeatureDigestDict[codename] = protocolFeature
                    break
        return protocolFeatureDigestDict

    def waitForHeadToAdvance(self, blocksToAdvance=1, timeout=None):
        currentHead = self.getHeadBlockNum()
        if timeout is None:
            timeout = 6 + blocksToAdvance / 2
        def isHeadAdvancing():
            return self.getHeadBlockNum() >= currentHead + blocksToAdvance
        return Utils.waitForTruth(isHeadAdvancing, timeout)

    def waitForLibToAdvance(self, timeout=30):
        currentLib = self.getIrreversibleBlockNum()
        def isLibAdvancing():
            return self.getIrreversibleBlockNum() > currentLib
        return Utils.waitForTruth(isLibAdvancing, timeout)

    # Require producer_api_plugin
    def activatePreactivateFeature(self):
        protocolFeatureDigestDict = self.getSupportedProtocolFeatureDict()
        preactivateFeatureDigest = protocolFeatureDigestDict["PREACTIVATE_FEATURE"]["feature_digest"]
        assert preactivateFeatureDigest

        self.scheduleProtocolFeatureActivations([preactivateFeatureDigest])

        # Wait for the next block to be produced so the scheduled protocol feature is activated
        assert self.waitForHeadToAdvance(blocksToAdvance=2), print("ERROR: TIMEOUT WAITING FOR PREACTIVATE")

    # Return an array of feature digests to be preactivated in a correct order respecting dependencies
    # Require producer_api_plugin
    def getAllBuiltinFeatureDigestsToPreactivate(self):
        protocolFeatures = []
        supportedProtocolFeatures = self.getSupportedProtocolFeatures()
        for protocolFeature in supportedProtocolFeatures:
            for spec in protocolFeature["specification"]:
                if (spec["name"] == "builtin_feature_codename"):
                    codename = spec["value"]
                    # Filter out "PREACTIVATE_FEATURE"
                    if codename != "PREACTIVATE_FEATURE":
                        protocolFeatures.append(protocolFeature["feature_digest"])
                    break
        return protocolFeatures

    # Require PREACTIVATE_FEATURE to be activated and require eosio.bios with preactivate_feature
    def preactivateProtocolFeatures(self, featureDigests:list):
        for digest in featureDigests:
            Utils.Print("push activate action with digest {}".format(digest))
            data="{{\"feature_digest\":{}}}".format(digest)
            opts="--permission eosio@active"
            trans=self.pushMessage("eosio", "activate", data, opts)
            if trans is None or not trans[0]:
                Utils.Print("ERROR: Failed to preactive digest {}".format(digest))
                return None
        self.waitForHeadToAdvance(blocksToAdvance=2)

    # Require PREACTIVATE_FEATURE to be activated and require eosio.bios with preactivate_feature
    def preactivateAllBuiltinProtocolFeature(self):
        allBuiltinProtocolFeatureDigests = self.getAllBuiltinFeatureDigestsToPreactivate()
        self.preactivateProtocolFeatures(allBuiltinProtocolFeatureDigests)

    def getLatestBlockHeaderState(self):
        headBlockNum = self.getHeadBlockNum()
        cmdDesc = "get block {} --header-state".format(headBlockNum)
        latestBlockHeaderState = self.processCleosCmd(cmdDesc, cmdDesc)
        return latestBlockHeaderState

    def getActivatedProtocolFeatures(self):
        latestBlockHeaderState = self.getLatestBlockHeaderState()
        return latestBlockHeaderState["activated_protocol_features"]["protocol_features"]

    def modifyBuiltinPFSubjRestrictions(self, featureCodename, subjectiveRestriction={}):
        jsonPath = os.path.join(Utils.getNodeConfigDir(self.nodeId),
                                "protocol_features",
                                "BUILTIN-{}.json".format(featureCodename))
        protocolFeatureJson = []
        with open(jsonPath) as f:
            protocolFeatureJson = json.load(f)
        protocolFeatureJson["subjective_restrictions"].update(subjectiveRestriction)
        with open(jsonPath, "w") as f:
            json.dump(protocolFeatureJson, f, indent=2)

    # Require producer_api_plugin
    def createSnapshot(self):
        param = { }
        return self.processCurlCmd("producer", "create_snapshot", json.dumps(param))

    def getScope(self, contract, table=None, limit=None, lower=None, upper=None, reverse=False, exitOnError=False):
        cmdDesc = "get scope"
        cmd="%s %s " % (cmdDesc, contract)
        if table:
            cmd+="--table %s " % (table)
        if limit:
            cmd+="--limit %s " % (limit)
        if lower:
            cmd+="--lower %s " % (lower)
        if upper:
            cmd+="--upper %s " % (upper)
        if reverse:
            cmd+="--reverse "
        msg="contract=%s" % (contract);
        return self.processCleosCmd(cmd, cmdDesc, exitOnError=exitOnError, exitMsg=msg)

    # kill all exsiting nodeos in case lingering from previous test
    @staticmethod
    def killAllNodeos():
        # kill the eos server
        cmd="pkill -9 %s" % (Utils.EosServerName)
        ret_code = subprocess.call(cmd.split(), stdout=Utils.FNull)
        Utils.Print("cmd: %s, ret:%d" % (cmd, ret_code))

    @staticmethod
    def findStderrFiles(path):
        files=[]
        it=os.scandir(path)
        for entry in it:
            if entry.is_file(follow_symlinks=False):
                match=re.match("stderr\..+\.txt", entry.name)
                if match:
                    files.append(os.path.join(path, entry.name))
        files.sort()
        return files

    def analyzeProduction(self, specificBlockNum=None, thresholdMs=500):
        dataDir=Utils.getNodeDataDir(self.nodeId)
        files=Node.findStderrFiles(dataDir)
        blockAnalysis={}
        anyBlockStr=r'[0-9]+'
        initialTimestamp=r'\s+([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3})\s'
        producedBlockPreStr=r'.+Produced\sblock\s+.+\s#('
        producedBlockPostStr=r')\s@\s([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}.[0-9]{3})'
        anyBlockPtrn=re.compile(initialTimestamp + producedBlockPreStr + anyBlockStr + producedBlockPostStr)
        producedBlockPtrn=re.compile(initialTimestamp + producedBlockPreStr + str(specificBlockNum) + producedBlockPostStr) if specificBlockNum is not None else anyBlockPtrn
        producedBlockDonePtrn=re.compile(initialTimestamp + r'.+Producing\sBlock\s+#' + anyBlockStr + '\sreturned:\strue')
        for file in files:
            with open(file, 'r') as f:
                line = f.readline()
                while line:
                    readLine=True  # assume we need to read the next line before the next pass
                    match = producedBlockPtrn.search(line)
                    if match:
                        prodTimeStr = match.group(1)
                        slotTimeStr = match.group(3)
                        blockNum = int(match.group(2))

                        line = f.readline()
                        while line:
                            matchNextBlock = anyBlockPtrn.search(line)
                            if matchNextBlock:
                                readLine=False  #already have the next line ready to check on next pass
                                break

                            matchBlockActuallyProduced = producedBlockDonePtrn.search(line)
                            if matchBlockActuallyProduced:
                                prodTimeStr = matchBlockActuallyProduced.group(1)
                                break

                            line = f.readline()

                        prodTime = datetime.strptime(prodTimeStr, Utils.TimeFmt)
                        slotTime = datetime.strptime(slotTimeStr, Utils.TimeFmt)
                        delta = prodTime - slotTime
                        limit = timedelta(milliseconds=thresholdMs)
                        if delta > limit:
                            if blockNum in blockAnalysis:
                                Utils.errorExit("Found repeat production of the same block num: %d in one of the stderr files in: %s" % (blockNum, dataDir))
                            blockAnalysis[blockNum] = { "slot": slotTimeStr, "prod": prodTimeStr }

                        if specificBlockNum is not None:
                            return blockAnalysis

                    if readLine:
                        line = f.readline()

        if specificBlockNum is not None and specificBlockNum not in blockAnalysis:
            blockAnalysis[specificBlockNum] = { "slot": None, "prod": None}

        return blockAnalysis

    def hasProducedBlockInRange(self, blockProducer, range):
        """Returns if any block in the specified range is produced by the specified producer"""
        try: 
            for blockNum in reversed(range):
                if blockProducer == self.getBlockProducerByNum(blockNum):
                    return True
            return False
        except:
            return False

    def waitForIrreversibleBlockProducedBy(self, producer, startBlockNum=0, retry=10):
        """
            wait until a node has seen a least an irreversible block produced by the specified producer since the specified startBlockNum
        """
        while retry > 0:
            latestBlockNum = self.getIrreversibleBlockNum()
            if self.hasProducedBlockInRange(producer, range(startBlockNum, latestBlockNum + 1)):
                return True
            time.sleep(1)
            retry = retry - 1
            startBlockNum = latestBlockNum + 1
        return False
