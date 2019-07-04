import copy
import subprocess
import time
import glob
import shutil
import os
import re
import string
import signal
import datetime
import sys
import random
import json

from core_symbol import CORE_SYMBOL
from testUtils import Utils
from testUtils import Account
from testUtils import BlockLogAction
from Node import BlockType
from Node import Node
from WalletMgr import WalletMgr

# Protocol Feature Setup Policy
class PFSetupPolicy:
    NONE = 0
    PREACTIVATE_FEATURE_ONLY = 1
    FULL = 2 # This will only happen if the cluster is bootstrapped (i.e. dontBootstrap == False)
    @staticmethod
    def hasPreactivateFeature(policy):
        return policy == PFSetupPolicy.PREACTIVATE_FEATURE_ONLY or \
                policy == PFSetupPolicy.FULL
    @staticmethod
    def isValid(policy):
        return policy == PFSetupPolicy.NONE or \
               policy == PFSetupPolicy.PREACTIVATE_FEATURE_ONLY or \
               policy == PFSetupPolicy.FULL

# pylint: disable=too-many-instance-attributes
# pylint: disable=too-many-public-methods
class Cluster(object):
    __chainSyncStrategies=Utils.getChainStrategies()
    __chainSyncStrategy=None
    __WalletName="MyWallet"
    __localHost="localhost"
    __BiosHost="localhost"
    __BiosPort=-1
    __LauncherCmdArr=[]
    __bootlog=""
    __BootScript=""
    __totalNodes=1

    __portBase = Utils.portBase
    __cluster_stride = Utils.cluster_stride
    __node_stride = Utils.node_stride
    __port_type_p2p = Utils.port_type_p2p
    __port_type_http = Utils.port_type_http
    __port_type_keosd = Utils.port_type_keosd

    # pylint: disable=too-many-arguments
    # walletd [True|False] Is keosd running. If not load the wallet plugin
    def __init__(self, walletd=False, localCluster=True, host="localhost", port=-1, walletHost="localhost", walletPort=-1, enableMongo=False
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

        Cluster.__BiosPort = Cluster.__portBase + Utils.clusterID * Cluster.__cluster_stride + Cluster.__port_type_http
        Cluster.__BootScript = ("cluster%05d/bios_boot.sh" % Utils.clusterID)

        Cluster.__bootlog=("cluster%05d/eosio-ignition-wd/bootlog.txt" % Utils.clusterID)
        
        if port == -1:
            port = Cluster.__BiosPort + Cluster.__node_stride

        if walletPort == -1:
            walletPort = Cluster.__portBase + Utils.clusterID * Cluster.__cluster_stride + Cluster.__port_type_keosd

        self.nodes={}
        self.unstartedNodes=[]
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
        self.mongoEndpointArgs=""
        self.mongoUri=""
        if self.enableMongo:
            if Utils.clusterID != 0:
                Utils.errorExit("tests with mongodb enabled must use clusterID 0")
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

        self.useBiosBootFile=False
        self.filesToCleanup=[]
        self.alternateVersionLabels=Cluster.__defaultAlternateVersionLabels()


    def setChainStrategy(self, chainSyncStrategy=Utils.SyncReplayTag):
        self.__chainSyncStrategy=self.__chainSyncStrategies.get(chainSyncStrategy)
        if self.__chainSyncStrategy is None:
            self.__chainSyncStrategy=self.__chainSyncStrategies.get("none")

    def setWalletMgr(self, walletMgr):
        self.walletMgr=walletMgr

    @staticmethod
    def __defaultAlternateVersionLabels():
        """Return a labels dictionary with just the "current" label to path set."""
        labels={}
        labels["current"]="./"
        return labels

    def setAlternateVersionLabels(self, file):
        """From the provided file return a dictionary of labels to paths."""
        Utils.Print("alternate file=%s" % (file))
        self.alternateVersionLabels=Cluster.__defaultAlternateVersionLabels()
        if file is None:
            # only have "current"
            return
        if not os.path.exists(file):
            Utils.errorExit("Alternate Version Labels file \"%s\" does not exist" % (file))
        with open(file, 'r') as f:
            content=f.read()
            p=re.compile(r'^\s*(\w+)\s*=\s*([^\s](?:.*[^\s])?)\s*$', re.MULTILINE)
            all=p.findall(content)
            for match in all:
                label=match[0]
                path=match[1]
                if label=="current":
                    Utils.Print("ERROR: cannot overwrite default label %s with path=%s" % (label, path))
                    continue
                self.alternateVersionLabels[label]=path
                if Utils.Debug: Utils.Print("Version label \"%s\" maps to \"%s\"" % (label, path))

    # launch local nodes and set self.nodes
    # pylint: disable=too-many-locals
    # pylint: disable=too-many-return-statements
    # pylint: disable=too-many-branches
    # pylint: disable=too-many-statements
    def launch(self, pnodes=1, unstartedNodes=0, totalNodes=1, prodCount=1, topo="mesh", delay=1, onlyBios=False, dontBootstrap=False,
               totalProducers=None, extraNodeosArgs=None, useBiosBootFile=True, specificExtraNodeosArgs=None, onlySetProds=False,
               pfSetupPolicy=PFSetupPolicy.FULL, alternateVersionLabelsFile=None, associatedNodeLabels=None, loadSystemContract=True):
        """Launch cluster.
        pnodes: producer nodes count
        unstartedNodes: non-producer nodes that are configured into the launch, but not started.  Should be included in totalNodes.
        totalNodes: producer + non-producer nodes + unstarted non-producer nodes count
        prodCount: producers per producer node count
        topo: cluster topology (as defined by launcher, and "bridge" shape that is specific to this launch method)
        delay: delay between individual nodes launch (as defined by launcher)
          delay 0 exposes a bootstrap bug where producer handover may have a large gap confusing nodes and bringing system to a halt.
        onlyBios: When true, only loads the bios contract (and not more full bootstrapping).
        dontBootstrap: When true, don't do any bootstrapping at all. (even bios is not uploaded)
        extraNodeosArgs: string of arguments to pass through to each nodoes instance (via --nodeos flag on launcher)
        useBiosBootFile: determines which of two bootstrap methods is used (when both dontBootstrap and onlyBios are false).
          The default value of true uses the bios_boot.sh file generated by the launcher.
          A value of false uses manual bootstrapping in this script, which does not do things like stake votes for producers.
        specificExtraNodeosArgs: dictionary of arguments to pass to a specific node (via --specific-num and
                                 --specific-nodeos flags on launcher), example: { "5" : "--plugin eosio::test_control_api_plugin" }
        onlySetProds: Stop the bootstrap process after setting the producers (only if useBiosBootFile is false)
        pfSetupPolicy: determine the protocol feature setup policy (none, preactivate_feature_only, or full)
        alternateVersionLabelsFile: Supply an alternate version labels file to use with associatedNodeLabels.
        associatedNodeLabels: Supply a dictionary of node numbers to use an alternate label for a specific node.
        loadSystemContract: indicate whether the eosio.system contract should be loaded (setting this to False causes useBiosBootFile to be treated as False)
        """
        assert(isinstance(topo, str))
        assert PFSetupPolicy.isValid(pfSetupPolicy)
        if alternateVersionLabelsFile is not None:
            assert(isinstance(alternateVersionLabelsFile, str))
        elif associatedNodeLabels is not None:
            associatedNodeLabels=None    # need to supply alternateVersionLabelsFile to use labels

        if associatedNodeLabels is not None:
            assert(isinstance(associatedNodeLabels, dict))
            Utils.Print("associatedNodeLabels size=%s" % (len(associatedNodeLabels)))
        Utils.Print("alternateVersionLabelsFile=%s" % (alternateVersionLabelsFile))

        if not self.localCluster:
            Utils.Print("WARNING: Cluster not local, not launching %s." % (Utils.EosServerName))
            return True

        if len(self.nodes) > 0:
            raise RuntimeError("Cluster already running.")

        Cluster.__totalNodes = totalNodes
        if pnodes > totalNodes:
            raise RuntimeError("totalNodes (%d) must be equal to or greater than pnodes(%d)." % (totalNodes, pnodes))
        if pnodes + unstartedNodes > totalNodes:
            raise RuntimeError("totalNodes (%d) must be equal to or greater than pnodes(%d) + unstartedNodes(%d)." % (totalNodes, pnodes, unstartedNodes))

        if self.walletMgr is None:
            self.walletMgr=WalletMgr(True)

        producerFlag=""
        if totalProducers:
            assert(isinstance(totalProducers, (str,int)))
            producerFlag="--producers %s" % (totalProducers)

        self.setAlternateVersionLabels(alternateVersionLabelsFile)

        tries = 30
        while not Utils.arePortsAvailable(set(range(Cluster.__BiosPort, Cluster.__BiosPort + (totalNodes+1) * Cluster.__node_stride))):
            Utils.Print("ERROR: Another process is listening on nodeos default port. wait...")
            if tries == 0:
                return False
            tries = tries - 1
            time.sleep(2)

        cmd="%s -p %s -n %s -d %s -i %s -f %s --unstarted-nodes %s" % (
            Utils.EosLauncherPath, pnodes, totalNodes, delay, datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%f")[:-3],
            producerFlag, unstartedNodes)
        cmd += " --cluster-id %d" % (Utils.clusterID)
        cmd += " --script %s" % (Cluster.__BootScript)
        cmdArr=cmd.split()
        if self.staging:
            cmdArr.append("--nogen")

        nodeosArgs="--max-transaction-time -1 --abi-serializer-max-time-ms 990000 --filter-on \"*\" --p2p-max-nodes-per-host %d" % (totalNodes)
        if not self.walletd:
            nodeosArgs += " --plugin eosio::wallet_api_plugin"
        if self.enableMongo:
            nodeosArgs += " --plugin eosio::mongo_db_plugin --mongodb-wipe --delete-all-blocks --mongodb-uri %s" % self.mongoUri
        if extraNodeosArgs is not None:
            assert(isinstance(extraNodeosArgs, str))
            nodeosArgs += extraNodeosArgs
        if Utils.Debug:
            nodeosArgs += " --contracts-console"
        if PFSetupPolicy.hasPreactivateFeature(pfSetupPolicy):
            nodeosArgs += " --plugin eosio::producer_api_plugin"

        if nodeosArgs:
            cmdArr.append("--nodeos")
            cmdArr.append(nodeosArgs)

        if specificExtraNodeosArgs is not None:
            assert(isinstance(specificExtraNodeosArgs, dict))
            for nodeNum,arg in specificExtraNodeosArgs.items():
                assert(isinstance(nodeNum, (str,int)))
                assert(isinstance(arg, str))
                cmdArr.append("--specific-num")
                cmdArr.append(str(nodeNum))
                cmdArr.append("--specific-nodeos")
                cmdArr.append(arg)

        cmdArr.append("--max-block-cpu-usage")
        cmdArr.append(str(160000000))
        cmdArr.append("--max-transaction-cpu-usage")
        cmdArr.append(str(150000000))

        if associatedNodeLabels is not None:
            for nodeNum,label in associatedNodeLabels.items():
                assert(isinstance(nodeNum, (str,int)))
                assert(isinstance(label, str))
                path=self.alternateVersionLabels.get(label)
                if path is None:
                    Utils.errorExit("associatedNodeLabels passed in indicates label %s for node num %s, but it was not identified in %s" % (label, nodeNum, alternateVersionLabelsFile))
                cmdArr.append("--spcfc-inst-num")
                cmdArr.append(str(nodeNum))
                cmdArr.append("--spcfc-inst-nodeos")
                cmdArr.append(path)

        # must be last cmdArr.append before subprocess.call, so that everything is on the command line
        # before constructing the shape.json file for "bridge"
        if topo=="bridge":
            shapeFilePrefix=("cluster%05d/shape_bridge" % Utils.clusterID)
            shapeFile=shapeFilePrefix+".json"
            cmdArrForOutput=copy.deepcopy(cmdArr)
            cmdArrForOutput.append("--output")
            cmdArrForOutput.append(shapeFile)
            s=" ".join(cmdArrForOutput)
            if Utils.Debug: Utils.Print("cmd: %s" % (s))
            if 0 != subprocess.call(cmdArrForOutput):
                Utils.Print("ERROR: Launcher failed to create shape file \"%s\"." % (shapeFile))
                return False

            f = open(shapeFile, "r")
            shapeFileJsonStr = f.read()
            f.close()
            shapeFileObject = json.loads(shapeFileJsonStr)
            Utils.Print("shapeFileObject=%s" % (shapeFileObject))
            # retrieve the nodes, which as a map of node name to node definition, which the fc library prints out as
            # an array of array, the first level of arrays is the pair entries of the map, the second is an array
            # of two entries - [ <first>, <second> ] with first being the name and second being the node definition
            shapeFileNodes = shapeFileObject["nodes"]

            numProducers=totalProducers if totalProducers is not None else (totalNodes - unstartedNodes)
            maxProducers=ord('z')-ord('a')+1
            assert numProducers<maxProducers, \
                   "ERROR: topo of %s assumes names of \"defproducera\" to \"defproducerz\", so must have at most %d producers" % \
                    (topo,maxProducers)

            # will make a map to node object to make identification easier
            biosNodeObject=None
            bridgeNodes={}
            producerNodes={}
            producers=[]
            for append in range(ord('a'),ord('a')+numProducers):
                name="defproducer" + chr(append)
                producers.append(name)

            # first group starts at 0
            secondGroupStart=int((numProducers+1)/2)
            producerGroup1=[]
            producerGroup2=[]

            Utils.Print("producers=%s" % (producers))
            shapeFileNodeMap = {}
            def getNodeNum(nodeName):
                p=re.compile(r'^testnet_(\d+)$')
                m=p.match(nodeName)
                return int(m.group(1))

            for shapeFileNodePair in shapeFileNodes:
                assert(len(shapeFileNodePair)==2)
                nodeName=shapeFileNodePair[0]
                shapeFileNode=shapeFileNodePair[1]
                shapeFileNodeMap[nodeName]=shapeFileNode
                Utils.Print("name=%s, shapeFileNode=%s" % (nodeName, shapeFileNodeMap[shapeFileNodePair[0]]))
                if nodeName=="bios":
                    biosNodeObject=shapeFileNode
                    continue
                nodeNum=getNodeNum(nodeName)
                Utils.Print("nodeNum=%d, shapeFileNode=%s" % (nodeNum, shapeFileNode))
                assert("producers" in shapeFileNode)
                shapeFileNodeProds=shapeFileNode["producers"]
                numNodeProducers=len(shapeFileNodeProds)
                if (numNodeProducers==0):
                    bridgeNodes[nodeName]=shapeFileNode
                else:
                    producerNodes[nodeName]=shapeFileNode
                    group=None
                    # go through all the producers for this node and determine which group on the bridged network they are in
                    for shapeFileNodeProd in shapeFileNodeProds:
                        producerIndex=0
                        for prod in producers:
                            if prod==shapeFileNodeProd:
                                break
                            producerIndex+=1

                        prodGroup=None
                        if producerIndex<secondGroupStart:
                            prodGroup=1
                            if group is None:
                                group=prodGroup
                                producerGroup1.append(nodeName)
                                Utils.Print("Group1 grouping producerIndex=%s, secondGroupStart=%s" % (producerIndex,secondGroupStart))
                        else:
                            prodGroup=2
                            if group is None:
                                group=prodGroup
                                producerGroup2.append(nodeName)
                                Utils.Print("Group2 grouping producerIndex=%s, secondGroupStart=%s" % (producerIndex,secondGroupStart))
                        if group!=prodGroup:
                            Utils.errorExit("Node configuration not consistent with \"bridge\" topology. Node %s has producers that fall into both halves of the bridged network" % (nodeName))

            for _,bridgeNode in bridgeNodes.items():
                bridgeNode["peers"]=[]
                for prodName in producerNodes:
                    bridgeNode["peers"].append(prodName)

            def connectGroup(group, producerNodes, bridgeNodes) :
                groupStr=""
                for nodeName in group:
                    groupStr+=nodeName+", "
                    prodNode=producerNodes[nodeName]
                    prodNode["peers"]=[i for i in group if i!=nodeName]
                    for bridgeName in bridgeNodes:
                        prodNode["peers"].append(bridgeName)

            connectGroup(producerGroup1, producerNodes, bridgeNodes)
            connectGroup(producerGroup2, producerNodes, bridgeNodes)

            f=open(shapeFile,"w")
            f.write(json.dumps(shapeFileObject, indent=4, sort_keys=True))
            f.close()

            cmdArr.append("--shape")
            cmdArr.append(shapeFile)
        else:
            cmdArr.append("--shape")
            cmdArr.append(topo)

        Cluster.__LauncherCmdArr = cmdArr.copy()

        s=" ".join([("'{0}'".format(element) if (' ' in element) else element) for element in cmdArr.copy()])
        if Utils.Debug: Utils.Print("cmd: %s" % (s))
        if 0 != subprocess.call(cmdArr):
            Utils.Print("ERROR: Launcher failed to launch. failed cmd: %s" % (s))
            return False

        startedNodes=totalNodes-unstartedNodes
        self.nodes=list(range(startedNodes)) # placeholder for cleanup purposes only

        nodes=self.discoverLocalNodes(startedNodes, timeout=Utils.systemWaitTimeout)
        if nodes is None or startedNodes != len(nodes):
            Utils.Print("ERROR: Unable to validate %s instances, expected: %d, actual: %d" %
                          (Utils.EosServerName, startedNodes, len(nodes)))
            return False

        self.nodes=nodes

        if unstartedNodes > 0:
            self.unstartedNodes=self.discoverUnstartedLocalNodes(unstartedNodes, totalNodes)

        biosNode=self.discoverBiosNode(timeout=Utils.systemWaitTimeout)
        if not biosNode or not Utils.waitForBool(biosNode.checkPulse, Utils.systemWaitTimeout):
            Utils.Print("ERROR: Bios node doesn't appear to be running...")
            return False

        if onlyBios:
            self.nodes=[biosNode]

        # ensure cluster node are inter-connected by ensuring everyone has block 1
        Utils.Print("Cluster viability smoke test. Validate every cluster node has block 1. ")
        if not self.waitOnClusterBlockNumSync(1):
            Utils.Print("ERROR: Cluster doesn't seem to be in sync. Some nodes missing block 1")
            return False

        if PFSetupPolicy.hasPreactivateFeature(pfSetupPolicy):
            Utils.Print("Activate Preactivate Feature.")
            biosNode.activatePreactivateFeature()

        if dontBootstrap:
            Utils.Print("Skipping bootstrap.")
            self.biosNode=biosNode
            return True

        Utils.Print("Bootstrap cluster.")
        if not loadSystemContract:
            useBiosBootFile=False  #ensure we use Cluster.bootstrap
        if onlyBios or not useBiosBootFile:
            self.biosNode=self.bootstrap(biosNode, startedNodes, prodCount, totalProducers, pfSetupPolicy, onlyBios, onlySetProds, loadSystemContract)
            if self.biosNode is None:
                Utils.Print("ERROR: Bootstrap failed: bootstrap failed")
                return False
        else:
            self.useBiosBootFile=True
            self.biosNode=self.bios_bootstrap(biosNode, startedNodes, pfSetupPolicy)
            if self.biosNode is None:
                Utils.Print("ERROR: Bootstrap failed: bios_bootstrap failed")
                return False

        if self.biosNode is None:
            Utils.Print("ERROR: Bootstrap failed:biosNode is None")
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

    # Initialize the default nodes (at present just the root node)
    def initializeNodes(self, defproduceraPrvtKey=None, defproducerbPrvtKey=None, onlyBios=False):
        port=Cluster.__BiosPort if onlyBios else self.port
        host=Cluster.__BiosHost if onlyBios else self.host
        node=Node(host, port, walletMgr=self.walletMgr, enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
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
            node=Node(host, port, walletMgr=self.walletMgr)
            if Utils.Debug: Utils.Print("Node:", node)

            node.checkPulse(exitOnError=True)
            nodes.append(node)

        self.nodes=nodes
        return True

    def setNodes(self, nodes):
        """manually set nodes, alternative to explicit launch"""
        self.nodes=nodes

    def waitOnClusterSync(self, timeout=None, blockType=BlockType.head, blockAdvancing=0):
        """Get head or irrevercible block on node 0, then ensure that block (or that block plus the
           blockAdvancing) is present on every cluster node."""
        assert(self.nodes)
        assert(len(self.nodes) > 0)
        node=self.nodes[0]
        targetBlockNum=node.getBlockNum(blockType) #retrieve node 0's head or irrevercible block number
        targetBlockNum+=blockAdvancing
        if Utils.Debug:
            Utils.Print("%s block number on root node: %d" % (blockType.type, targetBlockNum))
        if targetBlockNum == -1:
            return False

        return self.waitOnClusterBlockNumSync(targetBlockNum, timeout)

    def waitOnClusterBlockNumSync(self, targetBlockNum, timeout=None, blockType=BlockType.head):
        """Wait for all nodes to have targetBlockNum finalized."""
        assert(self.nodes)

        def doNodesHaveBlockNum(nodes, targetBlockNum, blockType, printCount):
            ret=True
            for node in nodes:
                try:
                    if (not node.killed) and (not node.isBlockPresent(targetBlockNum, blockType=blockType)):
                        ret=False
                        break
                except (TypeError) as _:
                    # This can happen if client connects before server is listening
                    ret=False
                    break

            printCount+=1
            if Utils.Debug and not ret and printCount%5==0:
                blockNums=[]
                for i in range(0, len(nodes)):
                    blockNums.append(nodes[i].getBlockNum())
                Utils.Print("Cluster still not in sync, head blocks for nodes: [ %s ]" % (", ".join(blockNums)))
            return ret

        printCount=0
        lam = lambda: doNodesHaveBlockNum(self.nodes, targetBlockNum, blockType, printCount)
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
            Utils.errorExit("Failed to retrieve node %d" % (nodeId))
        if exitOnError and self.nodes[nodeId] is None:
            Utils.cmdError("cluster has None value for node %d" % (nodeId))
            Utils.errorExit("Failed to retrieve node %d" % (nodeId))
        return self.nodes[nodeId]

    def getNodes(self):
        return self.nodes

    def launchUnstarted(self, numToLaunch=1, cachePopen=False):
        assert(isinstance(numToLaunch, int))
        assert(numToLaunch>0)
        launchList=self.unstartedNodes[:numToLaunch]
        del self.unstartedNodes[:numToLaunch]
        for node in launchList:
            # the node number is indexed off of the started nodes list
            node.launchUnstarted(len(self.nodes), cachePopen=cachePopen)
            self.nodes.append(node)

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

    def createAccountAndVerify(self, account, creator, stakedDeposit=1000, stakeNet=100, stakeCPU=100, buyRAM=10000):
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

    def createInitializeAccount(self, account, creatorAccount, stakedDeposit=1000, waitForTransBlock=False, stakeNet=100, stakeCPU=100, buyRAM=10000, exitOnError=False):
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
        regMsg="None" if m is None else "NOT None"
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

        configFile=Utils.getNodeConfigDir(nodeNum, "config.ini")
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

        configFile=Utils.getNodeConfigDir("bios", "config.ini")
        if Utils.Debug: Utils.Print("Parsing config file %s" % configFile)
        nodeName=Utils.nodeExtensionToName("bios")
        producerKeys=Cluster.parseProducerKeys(configFile, nodeName)
        if producerKeys is None:
            Utils.Print("ERROR: Failed to parse eosio private keys from cluster config files.")
            return None

        for i in range(0, totalNodes):
            configFile=Utils.getNodeConfigDir(i, "config.ini")
            if Utils.Debug: Utils.Print("Parsing config file %s" % configFile)

            nodeName=Utils.nodeExtensionToName(i)
            keys=Cluster.parseProducerKeys(configFile, nodeName)
            if keys is not None:
                producerKeys.update(keys)
            keyMsg="None" if keys is None else len(keys)

        return producerKeys

    def bios_bootstrap(self, biosNode, totalNodes, pfSetupPolicy, silent=False):
        """Bootstrap cluster using the bios_boot.sh script generated by eosio-launcher."""

        Utils.Print("Starting cluster bootstrap.")
        assert PFSetupPolicy.isValid(pfSetupPolicy)

        cmd="bash " + Cluster.__BootScript
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        env = {
            "BIOS_CONTRACT_PATH": "unittests/contracts/old_versions/v1.6.0-rc3/eosio.bios",
            "FEATURE_DIGESTS": ""
        }
        if PFSetupPolicy.hasPreactivateFeature(pfSetupPolicy):
            env["BIOS_CONTRACT_PATH"] = "unittests/contracts/eosio.bios"

        if pfSetupPolicy == PFSetupPolicy.FULL:
            allBuiltinProtocolFeatureDigests = biosNode.getAllBuiltinFeatureDigestsToPreactivate()
            env["FEATURE_DIGESTS"] = " ".join(allBuiltinProtocolFeatureDigests)
            Utils.Print("Set FEATURE_DIGESTS to: %s" % env["FEATURE_DIGESTS"])

        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull, env=env):
            if not silent: Utils.Print("Launcher failed to shut down eos cluster. The failing cmd is \"%s\"" % (cmd))
            return None

        p = re.compile('error', re.IGNORECASE)
        with open(Cluster.__bootlog) as bootFile:
            for line in bootFile:
                if p.search(line):
                    Utils.Print("ERROR: %s script resulted in errors. See %s" % (Cluster.__BootScript, Cluster.__bootlog))
                    Utils.Print(line)
                    return None

        producerKeys=Cluster.parseClusterKeys(totalNodes)
        # should have totalNodes node plus bios node
        if producerKeys is None or len(producerKeys) < (totalNodes+1):
            Utils.Print("ERROR: Failed to parse private keys from cluster config files.")
            return None

        self.walletMgr.killall()
        self.walletMgr.cleanup()

        if not self.walletMgr.launch():
            Utils.Print("ERROR: Failed to launch bootstrap wallet.")
            return None

        ignWallet=self.walletMgr.create("ignition")
        if ignWallet is None:
            Utils.Print("ERROR: Failed to create ignition wallet.")
            return None

        eosioName="eosio"
        eosioKeys=producerKeys[eosioName]
        eosioAccount=Account(eosioName)
        eosioAccount.ownerPrivateKey=eosioKeys["private"]
        eosioAccount.ownerPublicKey=eosioKeys["public"]
        eosioAccount.activePrivateKey=eosioKeys["private"]
        eosioAccount.activePublicKey=eosioKeys["public"]
        producerKeys.pop(eosioName)

        if not self.walletMgr.importKey(eosioAccount, ignWallet):
            Utils.Print("ERROR: Failed to import %s account keys into ignition wallet." % (eosioName))
            return None

        initialFunds="1000000.0000 {0}".format(CORE_SYMBOL)
        Utils.Print("Transfer initial fund %s to individual accounts." % (initialFunds))
        trans=None
        contract="eosio.token"
        action="transfer"
        for name, keys in producerKeys.items():
            data="{\"from\":\"eosio\",\"to\":\"%s\",\"quantity\":\"%s\",\"memo\":\"%s\"}" % (name, initialFunds, "init eosio transfer")
            opts="--permission eosio@active"
            if name != "eosio":
                trans=biosNode.pushMessage(contract, action, data, opts)
                if trans is None or not trans[0]:
                    Utils.Print("ERROR: Failed to transfer funds from eosio.token to %s." % (name))
                    return None

            Node.validateTransaction(trans[1])

        Utils.Print("Wait for last transfer transaction to become finalized.")
        transId=Node.getTransId(trans[1])
        if not biosNode.waitForTransInBlock(transId):
            Utils.Print("ERROR: Failed to validate transaction %s got rolled into a block on server port %d." % (transId, biosNode.port))
            return None

        Utils.Print("Cluster bootstrap done.")

        return biosNode

    def bootstrap(self, biosNode, totalNodes, prodCount, totalProducers, pfSetupPolicy, onlyBios=False, onlySetProds=False, loadSystemContract=True):
        """Create 'prodCount' init accounts and deposits 10000000000 SYS in each. If prodCount is -1 will initialize all possible producers.
        Ensure nodes are inter-connected prior to this call. One way to validate this will be to check if every node has block 1."""

        Utils.Print("Starting cluster bootstrap.")
        assert PFSetupPolicy.isValid(pfSetupPolicy)
        if totalProducers is None:
            totalProducers=totalNodes

        producerKeys=Cluster.parseClusterKeys(totalNodes)
        # should have totalNodes node plus bios node
        if producerKeys is None:
            Utils.Print("ERROR: Failed to parse any producer keys from config files.")
            return None
        elif len(producerKeys) < (totalProducers+1):
            Utils.Print("ERROR: Failed to parse %d producer keys from cluster config files, only found %d." % (totalProducers+1,len(producerKeys)))
            return None

        self.walletMgr.killall()
        self.walletMgr.cleanup()

        if not self.walletMgr.launch():
            Utils.Print("ERROR: Failed to launch bootstrap wallet.")
            return None

        ignWallet=self.walletMgr.create("ignition")

        eosioName="eosio"
        eosioKeys=producerKeys[eosioName]
        eosioAccount=Account(eosioName)
        eosioAccount.ownerPrivateKey=eosioKeys["private"]
        eosioAccount.ownerPublicKey=eosioKeys["public"]
        eosioAccount.activePrivateKey=eosioKeys["private"]
        eosioAccount.activePublicKey=eosioKeys["public"]

        if not self.walletMgr.importKey(eosioAccount, ignWallet):
            Utils.Print("ERROR: Failed to import %s account keys into ignition wallet." % (eosioName))
            return None

        contract="eosio.bios"
        contractDir="unittests/contracts/%s" % (contract)
        if PFSetupPolicy.hasPreactivateFeature(pfSetupPolicy):
            contractDir="unittests/contracts/%s" % (contract)
        else:
            contractDir="unittests/contracts/old_versions/v1.6.0-rc3/%s" % (contract)
        wasmFile="%s.wasm" % (contract)
        abiFile="%s.abi" % (contract)
        Utils.Print("Publish %s contract" % (contract))
        trans=biosNode.publishContract(eosioAccount.name, contractDir, wasmFile, abiFile, waitForTransBlock=True)
        if trans is None:
            Utils.Print("ERROR: Failed to publish contract %s." % (contract))
            return None

        if pfSetupPolicy == PFSetupPolicy.FULL:
            biosNode.preactivateAllBuiltinProtocolFeature()

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
                setProdsFile=("cluster%05d/setprods.json" % (Utils.clusterID))
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

        if onlySetProds: return biosNode

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
        contractDir="unittests/contracts/%s" % (contract)
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
        data="{\"issuer\":\"%s\",\"maximum_supply\":\"1000000000.0000 %s\"}" % (eosioAccount.name, CORE_SYMBOL)
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
        opts="--permission %s@active" % (eosioAccount.name)
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

        if loadSystemContract:
            contract="eosio.system"
            contractDir="unittests/contracts/%s" % (contract)
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

        # Only call init if the system contract is loaded
        if loadSystemContract:
            action="init"
            data="{\"version\":0,\"core\":\"4,%s\"}" % (CORE_SYMBOL)
            opts="--permission %s@active" % (eosioAccount.name)
            trans=biosNode.pushMessage(eosioAccount.name, action, data, opts)

        Utils.Print("Cluster bootstrap done.")

        return biosNode

    @staticmethod
    def pgrepEosServers(timeout=None):
        cmd=Utils.pgrepCmd(Utils.EosServerName)

        def myFunc():
            psOut=None
            try:
                if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
                psOut=Utils.checkOutput(cmd.split())
                return psOut
            except subprocess.CalledProcessError as ex:
                msg=ex.output.decode("utf-8")
                Utils.Print("ERROR: call of \"%s\" failed. %s" % (cmd, msg))
                return None
            return None

        return Utils.waitForObj(myFunc, timeout)

    @staticmethod
    def pgrepEosServerPattern(nodeInstance):
        dataLocation=Utils.getNodeDataDir(nodeInstance)
        return r"[\n]?(\d+) (.* --data-dir %s .*)\n" % (dataLocation)

    # Populates list of EosInstanceInfo objects, matched to actual running instances
    def discoverLocalNodes(self, totalNodes, timeout=None):
        nodes=[]

        psOut=Cluster.pgrepEosServers(timeout)
        if psOut is None:
            Utils.Print("ERROR: No nodes discovered.")
            return nodes

        if len(psOut) < 6660:
            psOutDisplay=psOut
        else:
            psOutDisplay=psOut[:6660]+"..."
        if Utils.Debug: Utils.Print("pgrep output: \"%s\"" % psOutDisplay)
        for i in range(0, totalNodes):
            instance=self.discoverLocalNode(i, psOut, timeout)
            if instance is None:
                break
            nodes.append(instance)

        if Utils.Debug: Utils.Print("Found %d nodes" % (len(nodes)))
        return nodes

    # Populate a node matched to actual running instance
    def discoverLocalNode(self, nodeNum, psOut=None, timeout=None):
        if psOut is None:
            psOut=Cluster.pgrepEosServers(timeout)
        if psOut is None:
            Utils.Print("ERROR: No nodes discovered.")
            return None
        pattern=Cluster.pgrepEosServerPattern(nodeNum)
        m=re.search(pattern, psOut, re.MULTILINE)
        if m is None:
            Utils.Print("ERROR: Failed to find %s pid. Pattern %s" % (Utils.EosServerName, pattern))
            return None
        instance=Node(self.host, self.port + Cluster.__node_stride * nodeNum, pid=int(m.group(1)), cmd=m.group(2), walletMgr=self.walletMgr, enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
        if Utils.Debug: Utils.Print("Node>", instance)
        return instance

    def discoverBiosNode(self, timeout=None):
        psOut=Cluster.pgrepEosServers(timeout=timeout)
        pattern=Cluster.pgrepEosServerPattern("bios")
        Utils.Print("pattern={\n%s\n}, psOut=\n%s\n" % (pattern,psOut))
        m=re.search(pattern, psOut, re.MULTILINE)
        if m is None:
            Utils.Print("ERROR: Failed to find %s pid. Pattern %s" % (Utils.EosServerName, pattern))
            return None
        else:
            return Node(Cluster.__BiosHost, Cluster.__BiosPort, pid=int(m.group(1)), cmd=m.group(2), walletMgr=self.walletMgr)

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

    def relaunchEosInstances(self, cachePopen=False):

        chainArg=self.__chainSyncStrategy.arg

        newChain= False if self.__chainSyncStrategy.name in [Utils.SyncHardReplayTag, Utils.SyncNoneTag] else True
        for i in range(0, len(self.nodes)):
            node=self.nodes[i]
            if node.killed and not node.relaunch(i, chainArg, newChain=newChain, cachePopen=cachePopen):
                return False

        return True

    @staticmethod
    def dumpErrorDetailImpl(fileName):
        Utils.Print(Utils.FileDivider)
        Utils.Print("Contents of %s:" % (fileName))
        if os.path.exists(fileName):
            with open(fileName, "r") as f:
                shutil.copyfileobj(f, sys.stdout)
        else:
            Utils.Print("File %s not found." % (fileName))

    @staticmethod
    def __findFiles(path):
        files=[]
        it=os.scandir(path)
        for entry in it:
            if entry.is_file(follow_symlinks=False):
                match=re.match("stderr\..+\.txt", entry.name)
                if match:
                    files.append(os.path.join(path, entry.name))
        files.sort()
        return files

    def dumpErrorDetails(self):
        fileName=Utils.getNodeConfigDir("bios", "config.ini")
        Cluster.dumpErrorDetailImpl(fileName)
        path=self.getNodeDataDir("bios")
        fileNames=Cluster.__findFiles(path)
        for fileName in fileNames:
            Cluster.dumpErrorDetailImpl(fileName)

        for i in range(0, len(self.nodes)):
            configLocation=Utils.getNodeConfigDir(i)
            fileName=os.path.join(configLocation, "config.ini")
            Cluster.dumpErrorDetailImpl(fileName)
            fileName=os.path.join(configLocation, "genesis.json")
            Cluster.dumpErrorDetailImpl(fileName)
            path=self.getNodeDataDir(i)
            fileNames=Cluster.__findFiles(path)
            for fileName in fileNames:
                Cluster.dumpErrorDetailImpl(fileName)

        if self.useBiosBootFile:
            Cluster.dumpErrorDetailImpl(Cluster.__bootlog)

    def killnode(self, index):
        pattern=Utils.getNodeDataDir(index)
        cmd=Utils.pgrepCmd(pattern)
        psOut=Utils.checkOutput(cmd.split(), ignoreError=True)
        if psOut is not None:
            m=psOut.split()
            if len(m) > 0:
                cmd="kill -9 %s" % (m[0])
                if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
                    if not silent: Utils.Print("Failed to kill nodeos")     

    def killall(self, silent=True, allInstances=False):
        """Kill cluster nodeos instances. allInstances will kill all nodeos instances running on the system."""
        cmd="%s -k 9" % (Utils.EosLauncherPath)
        cmd += " --cluster-id %d" % (Utils.clusterID)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            if not silent: Utils.Print("Launcher failed to shut down eos cluster.")
            
        if allInstances:
            # ocassionally the launcher cannot kill the eos server
            cmd="pkill -9 %s" % (Utils.EosServerName)
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
                if not silent: Utils.Print("Failed to shut down eos cluster.")
        else:
            self.killnode("bios")
            for i in range(0,Cluster.__totalNodes+1):
                self.killnode(i)

        # another explicit nodes shutdown
        for node in self.nodes:
            try:
                if node.pid is not None:
                    os.kill(node.pid, signal.SIGKILL)
            except OSError as _:
                pass

    def bounce(self, nodes, silent=True):
        """Bounces nodeos instances as indicated by parameter nodes.
        nodes should take the form of a comma-separated list as accepted by the launcher --bounce command (e.g. '00' or '00,01')"""
        cmdArr = Cluster.__LauncherCmdArr.copy()
        cmdArr.append("--bounce")
        cmdArr.append(nodes)
        cmd=" ".join(cmdArr)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmdArr):
            if not silent: Utils.Print("Launcher failed to bounce nodes: %s." % (nodes))
            return False
        return True

    def down(self, nodes, silent=True):
        """Brings down nodeos instances as indicated by parameter nodes.
        nodes should take the form of a comma-separated list as accepted by the launcher --bounce command (e.g. '00' or '00,01')"""
        cmdArr = Cluster.__LauncherCmdArr.copy()
        cmdArr.append("--down")
        cmdArr.append(nodes)
        cmd=" ".join(cmdArr)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmdArr):
            if not silent: Utils.Print("Launcher failed to take down nodes: %s." % (nodes))
            return False
        return True

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

    def getDataDir(self):
        return Utils.DataDir + "cluster%05d/" % (Utils.clusterID)

    def getConfigDir(self):
        return Utils.ConfigDir + "cluster%05d/" % (Utils.clusterID)

    def cleanup(self):
        for f in glob.glob(self.getDataDir()+ "node_*"):
            shutil.rmtree(f)
        for f in glob.glob(self.getConfigDir() + "node_*"):
            shutil.rmtree(f)

        for f in self.filesToCleanup:
            os.remove(f)

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

    def discoverUnstartedLocalNodes(self, unstartedNodes, totalNodes):
        unstarted=[]
        firstUnstartedNode=totalNodes-unstartedNodes
        for nodeId in range(firstUnstartedNode, totalNodes):
            unstarted.append(self.discoverUnstartedLocalNode(nodeId))
        return unstarted

    def discoverUnstartedLocalNode(self, nodeId):
        startFile=Node.unstartedFile(nodeId)
        with open(startFile, 'r') as file:
            cmd=file.read()
            Utils.Print("unstarted local node cmd: %s" % (cmd))
        p=re.compile(r'^\s*(\w+)\s*=\s*([^\s](?:.*[^\s])?)\s*$')
        instance=Node(self.host, port=self.port+Cluster.__node_stride, pid=None, cmd=cmd, walletMgr=self.walletMgr, enableMongo=self.enableMongo, mongoHost=self.mongoHost, mongoPort=self.mongoPort, mongoDb=self.mongoDb)
        if Utils.Debug: Utils.Print("Unstarted Node>", instance)
        return instance

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
                try:
                    node.reportStatus()
                except:
                    Utils.Print("No reportStatus")

    def printBlockLogIfNeeded(self):
        printBlockLog=False
        if hasattr(self, "nodes") and self.nodes is not None:
            for node in self.nodes:
                if node.missingTransaction:
                    printBlockLog=True
                    break

        if hasattr(self, "biosNode") and self.biosNode is not None and self.biosNode.missingTransaction:
            printBlockLog=True

        if not printBlockLog:
            return

        self.printBlockLog()

    def getBlockLog(self, nodeExtension, blockLogAction=BlockLogAction.return_blocks, outputFile=None, first=None, last=None, throwException=False, silentErrors=False, exitOnError=False):
        blockLogDir=Utils.getNodeDataDir(nodeExtension, "blocks")
        return Utils.getBlockLog(blockLogDir, blockLogAction=blockLogAction, outputFile=outputFile, first=first, last=last,  throwException=throwException, silentErrors=silentErrors, exitOnError=exitOnError)

    def printBlockLog(self):
        blockLogBios=self.getBlockLog("bios")
        Utils.Print(Utils.FileDivider)
        Utils.Print("Block log from %s:\n%s" % ("bios", json.dumps(blockLogBios, indent=1)))

        if not hasattr(self, "nodes"):
            return

        numNodes=len(self.nodes)
        for i in range(numNodes):
            node=self.nodes[i]
            blockLog=self.getBlockLog(i)
            Utils.Print(Utils.FileDivider)
            Utils.Print("Block log from node %s:\n%s" % (i, json.dumps(blockLog, indent=1)))


    def compareBlockLogs(self):
        blockLogs=[]
        blockNameExtensions=[]
        lowestMaxes=[]

        def back(arr):
            return arr[len(arr)-1]

        def sortLowest(maxes,max):
            for i in range(len(maxes)):
                if max < maxes[i]:
                    maxes.insert(i, max)
                    return

            maxes.append(max)

        i="bios"
        blockLog=self.getBlockLog(i)
        if blockLog is None:
            Utils.errorExit("Node %s does not have a block log, all nodes must have a block log" % (i))
        blockLogs.append(blockLog)
        blockNameExtensions.append(i)
        sortLowest(lowestMaxes,back(blockLog)["block_num"])

        if not hasattr(self, "nodes"):
            Utils.errorExit("There are not multiple nodes to compare, this method assumes that two nodes or more are expected")

        numNodes=len(self.nodes)
        for i in range(numNodes):
            node=self.nodes[i]
            blockLog=self.getBlockLog(i)
            if blockLog is None:
                Utils.errorExit("Node %s does not have a block log, all nodes must have a block log" % (i))
            blockLogs.append(blockLog)
            blockNameExtensions.append(i)
            sortLowest(lowestMaxes,back(blockLog)["block_num"])

        numNodes=len(blockLogs)

        if numNodes < 2:
            Utils.errorExit("There are not multiple nodes to compare, this method assumes that two nodes or more are expected")

        if lowestMaxes[0] < 2:
            Utils.errorExit("One or more nodes only has %d blocks, if that is a valid scenario, then compareBlockLogs shouldn't be called" % (lowestMaxes[0]))

        # create a list of block logs and name extensions for the given common block number span
        def identifyCommon(blockLogs, blockNameExtensions, first, last):
            commonBlockLogs=[]
            commonBlockNameExtensions=[]
            for i in range(numNodes):
                if (len(blockLogs[i]) >= last):
                    commonBlockLogs.append(blockLogs[i][first:last])
                    commonBlockNameExtensions.append(blockNameExtensions[i])
            return (commonBlockLogs,commonBlockNameExtensions)

        # compare the contents of the blockLogs for the given common block number span
        def compareCommon(blockLogs, blockNameExtensions, first, last):
            if Utils.Debug: Utils.Print("comparing block num %s through %s" % (first, last))
            commonBlockLogs=None
            commonBlockNameExtensions=None
            (commonBlockLogs,commonBlockNameExtensions) = identifyCommon(blockLogs, blockNameExtensions, first, last)
            numBlockLogs=len(commonBlockLogs)
            if numBlockLogs < 2:
                return False

            ret=None
            for i in range(1,numBlockLogs):
                context="<comparing block logs for node[%s] and node[%s]>" % (commonBlockNameExtensions[0], commonBlockNameExtensions[i])
                if Utils.Debug: Utils.Print("context=%s" % (context))
                ret=Utils.compare(commonBlockLogs[0], commonBlockLogs[i], context)
                if ret is not None:
                    blockLogDir1=self.getDataDir() + Utils.nodeExtensionToName(commonBlockNameExtensions[0]) + "/blocks/"
                    blockLogDir2=self.getDataDir() + Utils.nodeExtensionToName(commonBlockNameExtensions[i]) + "/blocks/"
                    Utils.Print(Utils.FileDivider)
                    Utils.Print("Block log from %s:\n%s" % (blockLogDir1, json.dumps(commonBlockLogs[0], indent=1)))
                    Utils.Print(Utils.FileDivider)
                    Utils.Print("Block log from %s:\n%s" % (blockLogDir2, json.dumps(commonBlockLogs[i], indent=1)))
                    Utils.Print(Utils.FileDivider)
                    Utils.errorExit("Block logs do not match, difference description -> %s" % (ret))

            return True

        def stripValues(lowestMaxes,greaterThan):
            newLowest=[]
            for low in lowestMaxes:
                if low > greaterThan:
                    newLowest.append(low)
            return newLowest

        first=0
        while len(lowestMaxes)>0 and compareCommon(blockLogs, blockNameExtensions, first, lowestMaxes[0]):
            first=lowestMaxes[0]+1
            lowestMaxes=stripValues(lowestMaxes,lowestMaxes[0])
