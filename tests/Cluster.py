import copy
import subprocess
import time
import glob
import shutil
import os
import platform
import re
import string
import signal
import datetime
import sys
import random
import json
import socket
import errno

from core_symbol import CORE_SYMBOL
from testUtils import Utils
from testUtils import Account
from Node import Node
from WalletMgr import WalletMgr

# pylint: disable=too-many-instance-attributes
# pylint: disable=too-many-public-methods
class Cluster(object):
    __chainSyncStrategies=Utils.getChainStrategies()
    __chainSyncStrategy=None
    __WalletName="MyWallet"
    __localHost="localhost"
    __BiosHost="localhost"
    __BiosPort=8788

    # pylint: disable=too-many-arguments
    # walletd [True|False] Is keosd running. If not load the wallet plugin
    def __init__(self, walletd=False, localCluster=True, host="localhost", port=8888, walletHost="localhost", walletPort=9899, enableMongo=False
                 , mongoHost="localhost", mongoPort=27017, mongoDb="EOStest", defproduceraPrvtKey=None, defproducerbPrvtKey=None, staging=False):
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
        self.defProducerAccounts={}
        self.defproduceraAccount=self.defProducerAccounts["defproducera"]= Account("defproducera")
        self.defproducerbAccount=self.defProducerAccounts["defproducerb"]= Account("defproducerb")
        self.eosioAccount=self.defProducerAccounts["eosio"]= Account("eosio")

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
    def launch(self, pnodes=1, totalNodes=1, prodCount=1, topo="mesh", p2pPlugin="net", delay=1, onlyBios=False, dontKill=False
               , dontBootstrap=False, totalProducers=None, extraNodeosArgs=None):
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

        producerFlag=""
        if totalProducers:
            assert(isinstance(totalProducers, (str,int)))
            producerFlag="--producers %s" % (totalProducers)

        tries = 30
        while not Cluster.arePortsAvailable(set(range(self.port, self.port+totalNodes+1))):
            Utils.Print("ERROR: Another process is listening on nodeos default port. wait...")
            if tries == 0:
                return False
            tries = tries - 1
            time.sleep(2)

        cmd="%s -p %s -n %s -s %s -d %s -i %s -f --p2p-plugin %s %s" % (
            Utils.EosLauncherPath, pnodes, totalNodes, topo, delay, datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
            p2pPlugin, producerFlag)
        cmdArr=cmd.split()
        if self.staging:
            cmdArr.append("--nogen")

        nodeosArgs="--max-transaction-time 50000 --abi-serializer-max-time-ms 990000 --filter-on * --p2p-max-nodes-per-host %d" % (totalNodes)
        if not self.walletd:
            nodeosArgs += " --plugin eosio::wallet_api_plugin"
        if self.enableMongo:
            nodeosArgs += " --plugin eosio::mongo_db_plugin --mongodb-wipe --delete-all-blocks --mongodb-uri %s" % self.mongoUri
        if extraNodeosArgs is not None:
            assert(isinstance(extraNodeosArgs, str))
            nodeosArgs += extraNodeosArgs
        if Utils.Debug:
            nodeosArgs += " --contracts-console"

        if nodeosArgs:
            cmdArr.append("--nodeos")
            cmdArr.append(nodeosArgs)

        s=" ".join(cmdArr)
        if Utils.Debug: Utils.Print("cmd: %s" % (s))
        if 0 != subprocess.call(cmdArr):
            Utils.Print("ERROR: Launcher failed to launch. failed cmd: %s" % (s))
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
        self.biosNode=Cluster.bootstrap(totalNodes, prodCount, Cluster.__BiosHost, Cluster.__BiosPort, dontKill, onlyBios)
        if self.biosNode is None:
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

        for name,_ in producerKeys.items():
            account=Account(name)
            initAccountKeys(account, producerKeys[name])
            self.defProducerAccounts[name] = account

        self.eosioAccount=self.defProducerAccounts["eosio"]
        self.defproduceraAccount=self.defProducerAccounts["defproducera"]
        self.defproducerbAccount=self.defProducerAccounts["defproducerb"]

        return True

    @staticmethod
    def arePortsAvailable(ports):
        """Check if specified ports are available for listening on."""
        assert(ports)
        assert(isinstance(ports, set))

        for port in ports:
            if Utils.Debug: Utils.Print("Checking if port %d is available." % (port))
            assert(isinstance(port, int))
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

            try:
                s.bind(("127.0.0.1", port))
            except socket.error as e:
                if e.errno == errno.EADDRINUSE:
                    Utils.Print("ERROR: Port %d is already in use" % (port))
                else:
                    # something else raised the socket.error exception
                    Utils.Print("ERROR: Unknown exception while trying to listen on port %d" % (port))
                    Utils.Print(e)
                return False
            finally:
                s.close()

        return True


    # Initialize the default nodes (at present just the root node)
    def initializeNodes(self, defproduceraPrvtKey=None, defproducerbPrvtKey=None, onlyBios=False):
        port=Cluster.__BiosPort if onlyBios else self.port
        host=Cluster.__BiosHost if onlyBios else self.host
        node=Node(host, port, enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
        node.setWalletEndpointArgs(self.walletEndpointArgs)
        if Utils.Debug: Utils.Print("Node: %s", str(node))

        node.checkPulse(exitOnError=True)
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

            node.checkPulse(exitOnError=True)
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
                        return False
                except (TypeError) as _:
                    # This can happen if client connects before server is listening
                    return False

            return True

        lam = lambda: doNodesHaveBlockNum(self.nodes, targetBlockNum)
        ret=Utils.waitForBool(lam, timeout)
        return ret

    @staticmethod
    def getClientVersion(verbose=False):
        """Returns client version (string)"""
        p = re.compile(r'^Build version:\s(\w+)\n$')
        try:
            cmd="%s version client" % (Utils.EosClientPath)
            if verbose: Utils.Print("cmd: %s" % (cmd))
            response=Utils.checkOutput(cmd.split())
            assert(response)
            assert(isinstance(response, str))
            if verbose: Utils.Print("response: <%s>" % (response))
            m=p.match(response)
            if m is None:
                Utils.Print("ERROR: client version regex mismatch")
                return None

            verStr=m.group(1)
            return verStr
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Exception during client version query. %s" % (msg))
            raise

    @staticmethod
    def createAccountKeys(count):
        accounts=[]
        p = re.compile('Private key: (.+)\nPublic key: (.+)\n', re.MULTILINE)
        for _ in range(0, count):
            try:
                cmd="%s create key --to-console" % (Utils.EosClientPath)
                if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
                keyStr=Utils.checkOutput(cmd.split())
                m=p.search(keyStr)
                if m is None:
                    Utils.Print("ERROR: Owner key creation regex mismatch")
                    break

                ownerPrivate=m.group(1)
                ownerPublic=m.group(2)

                cmd="%s create key --to-console" % (Utils.EosClientPath)
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

    def getNode(self, nodeId=0, exitOnError=True):
        if exitOnError and nodeId >= len(self.nodes):
            Utils.cmdError("cluster never created node %d" % (nodeId))
            errorExit("Failed to retrieve node %d" % (nodeId))
        if exitOnError and self.nodes[nodeId] is None:
            Utils.cmdError("cluster has None value for node %d" % (nodeId))
            errorExit("Failed to retrieve node %d" % (nodeId))
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
                Utils.Print("ERROR: Failed to validate transaction %s got rolled into a block on server port %d." % (transId, node.port))
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
            Utils.Print("ERROR: Failed to validate transaction %s got rolled into a block on server port %d." % (transId, node.port))
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

    def createAccountAndVerify(self, account, creator, stakedDeposit=1000, stakeNet=100, stakeCPU=100, buyRAM=100):
        """create account, verify account and return transaction id"""
        assert(len(self.nodes) > 0)
        node=self.nodes[0]
        trans=node.createInitializeAccount(account, creator, stakedDeposit, stakeNet=stakeNet, stakeCPU=stakeCPU, buyRAM=buyRAM, exitOnError=True)
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

    def createInitializeAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False, stakeNet=100, stakeCPU=100, buyRAM=100, exitOnError=False):
        assert(len(self.nodes) > 0)
        node=self.nodes[0]
        trans=node.createInitializeAccount(account, creatorAccount, stakedDeposit, waitForTransBlock, stakeNet=stakeNet, stakeCPU=stakeCPU, buyRAM=buyRAM)
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
    def parseProducers(nodeNum):
        """Parse node config file for producers."""

        node="node_%02d" % (nodeNum)
        configFile="etc/eosio/%s/config.ini" % (node)
        if Utils.Debug: Utils.Print("Parsing config file %s" % configFile)
        configStr=None
        with open(configFile, 'r') as f:
            configStr=f.read()

        pattern=r"^\s*producer-name\s*=\W*(\w+)\W*$"
        producerMatches=re.findall(pattern, configStr, re.MULTILINE)
        if producerMatches is None:
            if Utils.Debug: Utils.Print("Failed to find producers.")
            return None

        return producerMatches

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
            return None

        producerKeys=Cluster.parseClusterKeys(totalNodes)
        # should have totalNodes node plus bios node
        if producerKeys is None or len(producerKeys) < (totalNodes+1):
            Utils.Print("ERROR: Failed to parse private keys from cluster config files.")
            return None

        walletMgr=WalletMgr(True)
        walletMgr.killall()
        walletMgr.cleanup()

        if not walletMgr.launch():
            Utils.Print("ERROR: Failed to launch bootstrap wallet.")
            return None
        biosNode.setWalletEndpointArgs(walletMgr.walletEndpointArgs)

        try:
            ignWallet=walletMgr.create("ignition")

            eosioName="eosio"
            eosioKeys=producerKeys[eosioName]
            eosioAccount=Account(eosioName)
            eosioAccount.ownerPrivateKey=eosioKeys["private"]
            eosioAccount.ownerPublicKey=eosioKeys["public"]
            eosioAccount.activePrivateKey=eosioKeys["private"]
            eosioAccount.activePublicKey=eosioKeys["public"]

            if not walletMgr.importKey(eosioAccount, ignWallet):
                Utils.Print("ERROR: Failed to import %s account keys into ignition wallet." % (eosioName))
                return None

            contract="eosio.bios"
            contractDir="contracts/%s" % (contract)
            wasmFile="%s.wasm" % (contract)
            abiFile="%s.abi" % (contract)
            Utils.Print("Publish %s contract" % (contract))
            trans=biosNode.publishContract(eosioAccount.name, contractDir, wasmFile, abiFile, waitForTransBlock=True)
            if trans is None:
                Utils.Print("ERROR: Failed to publish contract %s." % (contract))
                return None

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
                    return None
                Node.validateTransaction(trans)
                accounts.append(initx)

            transId=Node.getTransId(trans)
            if not biosNode.waitForTransInBlock(transId):
                Utils.Print("ERROR: Failed to validate transaction %s got rolled into a block on server port %d." % (transId, biosNode.port))
                return None

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
                            return None
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
                        return None

                trans=trans[1]
                transId=Node.getTransId(trans)
                if not biosNode.waitForTransInBlock(transId):
                    Utils.Print("ERROR: Failed to validate transaction %s got rolled into a block on server port %d." % (transId, biosNode.port))
                    return None

                # wait for block production handover (essentially a block produced by anyone but eosio).
                lam = lambda: biosNode.getInfo(exitOnError=True)["head_block_producer"] != "eosio"
                ret=Utils.waitForBool(lam)
                if not ret:
                    Utils.Print("ERROR: Block production handover failed.")
                    return None

            eosioTokenAccount=copy.deepcopy(eosioAccount)
            eosioTokenAccount.name="eosio.token"
            trans=biosNode.createAccount(eosioTokenAccount, eosioAccount, 0)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s" % (eosioTokenAccount.name))
                return None

            eosioRamAccount=copy.deepcopy(eosioAccount)
            eosioRamAccount.name="eosio.ram"
            trans=biosNode.createAccount(eosioRamAccount, eosioAccount, 0)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s" % (eosioRamAccount.name))
                return None

            eosioRamfeeAccount=copy.deepcopy(eosioAccount)
            eosioRamfeeAccount.name="eosio.ramfee"
            trans=biosNode.createAccount(eosioRamfeeAccount, eosioAccount, 0)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s" % (eosioRamfeeAccount.name))
                return None

            eosioStakeAccount=copy.deepcopy(eosioAccount)
            eosioStakeAccount.name="eosio.stake"
            trans=biosNode.createAccount(eosioStakeAccount, eosioAccount, 0)
            if trans is None:
                Utils.Print("ERROR: Failed to create account %s" % (eosioStakeAccount.name))
                return None

            Node.validateTransaction(trans)
            transId=Node.getTransId(trans)
            if not biosNode.waitForTransInBlock(transId):
                Utils.Print("ERROR: Failed to validate transaction %s got rolled into a block on server port %d." % (transId, biosNode.port))
                return None

            contract="eosio.token"
            contractDir="contracts/%s" % (contract)
            wasmFile="%s.wasm" % (contract)
            abiFile="%s.abi" % (contract)
            Utils.Print("Publish %s contract" % (contract))
            trans=biosNode.publishContract(eosioTokenAccount.name, contractDir, wasmFile, abiFile, waitForTransBlock=True)
            if trans is None:
                Utils.Print("ERROR: Failed to publish contract %s." % (contract))
                return None

            # Create currency0000, followed by issue currency0000
            contract=eosioTokenAccount.name
            Utils.Print("push create action to %s contract" % (contract))
            action="create"
            data="{\"issuer\":\"%s\",\"maximum_supply\":\"1000000000.0000 %s\",\"can_freeze\":\"0\",\"can_recall\":\"0\",\"can_whitelist\":\"0\"}" % (eosioTokenAccount.name, CORE_SYMBOL)
            opts="--permission %s@active" % (contract)
            trans=biosNode.pushMessage(contract, action, data, opts)
            if trans is None or not trans[0]:
                Utils.Print("ERROR: Failed to push create action to eosio contract.")
                return None

            Node.validateTransaction(trans[1])
            transId=Node.getTransId(trans[1])
            if not biosNode.waitForTransInBlock(transId):
                Utils.Print("ERROR: Failed to validate transaction %s got rolled into a block on server port %d." % (transId, biosNode.port))
                return None

            contract=eosioTokenAccount.name
            Utils.Print("push issue action to %s contract" % (contract))
            action="issue"
            data="{\"to\":\"%s\",\"quantity\":\"1000000000.0000 %s\",\"memo\":\"initial issue\"}" % (eosioAccount.name, CORE_SYMBOL)
            opts="--permission %s@active" % (contract)
            trans=biosNode.pushMessage(contract, action, data, opts)
            if trans is None or not trans[0]:
                Utils.Print("ERROR: Failed to push issue action to eosio contract.")
                return None

            Node.validateTransaction(trans[1])
            Utils.Print("Wait for issue action transaction to become finalized.")
            transId=Node.getTransId(trans[1])
            # biosNode.waitForTransInBlock(transId)
            # guesstimating block finalization timeout. Two production rounds of 12 blocks per node, plus 60 seconds buffer
            timeout = .5 * 12 * 2 * len(producerKeys) + 60
            if not biosNode.waitForTransFinalization(transId, timeout=timeout):
                Utils.Print("ERROR: Failed to validate transaction %s got rolled into a finalized block on server port %d." % (transId, biosNode.port))
                return None

            expectedAmount="1000000000.0000 {0}".format(CORE_SYMBOL)
            Utils.Print("Verify eosio issue, Expected: %s" % (expectedAmount))
            actualAmount=biosNode.getAccountEosBalanceStr(eosioAccount.name)
            if expectedAmount != actualAmount:
                Utils.Print("ERROR: Issue verification failed. Excepted %s, actual: %s" %
                            (expectedAmount, actualAmount))
                return None

            contract="eosio.system"
            contractDir="contracts/%s" % (contract)
            wasmFile="%s.wasm" % (contract)
            abiFile="%s.abi" % (contract)
            Utils.Print("Publish %s contract" % (contract))
            trans=biosNode.publishContract(eosioAccount.name, contractDir, wasmFile, abiFile, waitForTransBlock=True)
            if trans is None:
                Utils.Print("ERROR: Failed to publish contract %s." % (contract))
                return None

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
                    return None

                Node.validateTransaction(trans[1])

            Utils.Print("Wait for last transfer transaction to become finalized.")
            transId=Node.getTransId(trans[1])
            if not biosNode.waitForTransInBlock(transId):
                Utils.Print("ERROR: Failed to validate transaction %s got rolled into a block on server port %d." % (transId, biosNode.port))
                return None

            Utils.Print("Cluster bootstrap done.")
        finally:
            if not dontKill:
                walletMgr.killall()
                walletMgr.cleanup()

        return biosNode


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
                Utils.Print("ERROR: Failed to validate transaction %s got rolled into a block on server port %d." % (transId, node.port))
                return False

        return True

    def getInfos(self, silentErrors=False, exitOnError=False):
        infos=[]
        for node in self.nodes:
            infos.append(node.getInfo(silentErrors=silentErrors, exitOnError=exitOnError))

        return infos

    def reportStatus(self):
        if hasattr(self, "biosNode") and self.biosNode is not None:
            self.biosNode.reportStatus()
        if hasattr(self, "nodes"): 
            for node in self.nodes:
                node.reportStatus()
