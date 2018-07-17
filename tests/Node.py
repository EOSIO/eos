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
    def runMongoCmdReturnJson(cmd, subcommand, trace=False):
        """Run mongodb subcommand and return response."""
        assert(cmd)
        assert(isinstance(cmd, list))
        assert(subcommand)
        assert(isinstance(subcommand, str))
        retId,outs,errs=Node.stdinAndCheckOutput(cmd, subcommand)
        if retId is not 0:
            Utils.Print("ERROR: mongodb call failed. %s" % (errs))
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
                if not self.enableMongo:
                    assert(accountInfo["account_name"] == account.name)
                else:
                    assert(accountInfo["name"] == account.name)
            except (AssertionError, TypeError, KeyError) as _:
                Utils.Print("account validation failed. account: %s" % (account.name))
                raise

    # pylint: disable=too-many-branches
    def getBlock(self, blockNum, silentErrors=False):
        """Given a blockId will return block details."""
        assert(isinstance(blockNum, int))
        if not self.enableMongo:
            cmd="%s %s get block %d" % (Utils.EosClientPath, self.endpointArgs, blockNum)
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            try:
                block=Utils.runCmdReturnJson(cmd)
                return block
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    msg=ex.output.decode("utf-8")
                    Utils.Print("ERROR: Exception during get block. %s" % (msg))
                return None
        else:
            cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
            subcommand='db.blocks.findOne( { "block_num": %d } )' % (blockNum)
            if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
            try:
                block=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
                if block is not None:
                    return block
            except subprocess.CalledProcessError as ex:
                if not silentErrors:
                    msg=ex.output.decode("utf-8")
                    Utils.Print("ERROR: Exception during get db node get block. %s" % (msg))
                return None

        return None

    def getBlockById(self, blockId, silentErrors=False):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand='db.blocks.findOne( { "block_id": "%s" } )' % (blockId)
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

        return None

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

        finalized  = True if blockNum <= node_block_num else False
        if Utils.Debug:
            if finalized:
                Utils.Print("Block %d is finalized." % (blockNum))
            else:
                Utils.Print("Block %d is not yet finalized." % (blockNum))

        return finalized

    # pylint: disable=too-many-branches
    def getTransaction(self, transId, silentErrors=False):
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
            return self.getTransactionMdb(transId, silentErrors)

        return None

    def getTransactionMdb(self, transId, silentErrors=False):
        """Get transaction from MongoDB. Since DB only contains finalized blocks, transactions can take a while to appear in DB."""
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        #subcommand='db.Transactions.findOne( { $and : [ { "trx_id": "%s" }, {"irreversible":true} ] } )' % (transId)
        subcommand='db.transactions.findOne( { "trx_id": "%s" } )' % (transId)
        if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
        try:
            trans=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
            return trans
        except subprocess.CalledProcessError as ex:
            if not silentErrors:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during get db node get trans. %s" % (msg))
            return None

    def isTransInBlock(self, transId, blockId):
        """Check if transId is within block identified by blockId"""
        assert(transId)
        assert(isinstance(transId, str))
        assert(blockId)
        assert(isinstance(blockId, int))

        block=self.getBlock(blockId)
        assert(block)
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

    def getBlockIdByTransId(self, transId):
        """Given a transaction Id (string), will return block id (int) containing the transaction"""
        assert(transId)
        assert(isinstance(transId, str))
        trans=self.getTransaction(transId)
        if trans is None:
            return None

        refBlockNum=None
        key=""
        try:
            if not self.enableMongo:
                key="[trx][trx][ref_block_num]"
                refBlockNum=trans["trx"]["trx"]["ref_block_num"]
            else:
                key="[transaction_header][ref_block_num]"
                refBlockNum=trans["transaction_header"]["ref_block_num"]
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
            refBlockNum=trans["transaction_header"]["ref_block_num"]
            refBlockNum=int(refBlockNum)+1
        except (TypeError, ValueError, KeyError) as _:
            Utils.Print("transaction[transaction_header][ref_block_num] not found. Transaction: %s" % (trans))
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
        return self.isBlockFinalized(blockId)

    # Create & initialize account and return creation transactions. Return transaction json object
    def createInitializeAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False, stakeNet=100, stakeCPU=100, buyRAM=100):
        cmd='%s %s system newaccount -j %s %s %s %s --stake-net "%s %s" --stake-cpu "%s %s" --buy-ram "%s %s"' % (
            Utils.EosClientPath, self.endpointArgs, creatorAccount.name, account.name,
            account.ownerPublicKey, account.activePublicKey,
            stakeNet, CORE_SYMBOL, stakeCPU, CORE_SYMBOL, buyRAM, CORE_SYMBOL)

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

    def createAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False):
        """Create account and return creation transactions. Return transaction json object.
        waitForTransBlock: wait on creation transaction id to appear in a block."""
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
        if not self.enableMongo:
            cmd="%s %s get account -j %s" % (Utils.EosClientPath, self.endpointArgs, name)
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            try:
                trans=Utils.runCmdReturnJson(cmd)
                return trans
            except subprocess.CalledProcessError as ex:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during get account. %s" % (msg))
                return None
        else:
            return self.getEosAccountFromDb(name)

    def getEosAccountFromDb(self, name):
        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand='db.accounts.findOne({"name" : "%s"})' % (name)
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
            print("transaction[rows][0][balance] not found. Transaction: %s" % (trans))
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

    def waitForNextBlock(self, timeout=None):
        num=self.getHeadBlockNum()
        lam = lambda: self.getHeadBlockNum() > num
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForBlock(self, blockNum, timeout=None):
        lam = lambda: self.getHeadBlockNum() > blockNum
        ret=Utils.waitForBool(lam, timeout)
        return ret

    def waitForIrreversibleBlock(self, blockNum, timeout=None):
        lam = lambda: self.getIrreversibleBlockNum() >= blockNum
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

        if not self.enableMongo:
            cmd="%s %s get actions -j %s %d %d" % (Utils.EosClientPath, self.endpointArgs, account.name, pos, offset)
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            try:
                actions=Utils.runCmdReturnJson(cmd)
                return actions
            except subprocess.CalledProcessError as ex:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: Exception during actions by account retrieval. %s" % (msg))
                return None
        else:
            return self.getActionsMdb(account, pos, offset)

    def getActionsMdb(self, account, pos=-1, offset=-1):
        assert(isinstance(account, Account))
        assert(isinstance(pos, int))
        assert(isinstance(offset, int))

        cmd="%s %s" % (Utils.MongoPath, self.mongoEndpointArgs)
        subcommand='db.actions.find({$or: [{"data.from":"%s"},{"data.to":"%s"}]}).sort({"_id":%d}).limit(%d)' % (account.name, account.name, pos, abs(offset))
        if Utils.Debug: Utils.Print("cmd: echo '%s' | %s" % (subcommand, cmd))
        try:
            actions=Node.runMongoCmdReturnJson(cmd.split(), subcommand)
            if actions is not None:
                return actions
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during get db actions. %s" % (msg))
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

    def delegatebw(self, fromAccount, netQuantity, cpuQuantity, toAccount=None, transferTo=False, waitForTransBlock=False):
        if toAccount is None:
            toAccount=fromAccount

        specificCmd="system delegatebw"
        transferStr="--transfer" if transferTo else "" 
        cmd="%s %s %s -j %s %s \"%s %s\" \"%s %s\" %s" % (
            Utils.EosClientPath, self.endpointArgs, specificCmd, fromAccount.name, toAccount.name, netQuantity, CORE_SYMBOL, cpuQuantity, CORE_SYMBOL, transferStr)
        return self.processCmd(cmd, specificCmd, waitForTransBlock)

    def regproducer(self, producer, url, location, waitForTransBlock=False):
        specificCmd="system regproducer"
        cmd="%s %s %s -j %s %s %s %s" % (
            Utils.EosClientPath, self.endpointArgs, specificCmd, producer.name, producer.activePublicKey, url, location)
        return self.processCmd(cmd, specificCmd, waitForTransBlock)

    def vote(self, account, producers, waitForTransBlock=False):
        specificCmd = "system voteproducer prods"
        cmd="%s %s %s -j %s %s" % (
            Utils.EosClientPath, self.endpointArgs, specificCmd, account.name, " ".join(producers))
        return self.processCmd(cmd, specificCmd, waitForTransBlock)

    def processCmd(self, cmd, cmdDesc, waitForTransBlock):
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        trans=None
        try:
            trans=Utils.runCmdReturnJson(cmd)
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during %s. %s" % (cmdDesc, msg))
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
        subcommand="db.blocks.find().sort({\"_id\":%d}).limit(1).pretty()" % (idx)
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
