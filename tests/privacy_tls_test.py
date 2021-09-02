#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from Node import Node
from TestHelper import TestHelper

import os

def makeRootCertArgs(privacyDir):
    privacyDir=os.path.join(Utils.ConfigDir, privacyDir)
    certAuth = os.path.join(privacyDir, "CA_cert.pem")
    nodeCert = os.path.join(privacyDir, "CA_cert.pem")
    nodeKey = os.path.join(privacyDir, "CA_key.pem")
    return "--p2p-tls-own-certificate-file {} --p2p-tls-private-key-file {} --p2p-tls-security-group-ca-file {}".format(nodeCert, nodeKey, certAuth)

def makeWrongPrivateKeyArgs(index):
    participantName = Node.participantName(index+1)
    certAuth = os.path.join(os.path.join(Utils.ConfigDir, "privacy1"), "CA_cert.pem")
    nodeCert = os.path.join(os.path.join(Utils.ConfigDir, "privacy1"), "{}.crt".format(participantName))
    nodeKey =  os.path.join(os.path.join(Utils.ConfigDir, "privacy2"), "{}_key.pem".format(participantName))
    return "--p2p-tls-own-certificate-file {} --p2p-tls-private-key-file {} --p2p-tls-security-group-ca-file {}".format(nodeCert, nodeKey, certAuth)

def makeNoCAArgs(index):
    participantName = Node.participantName(index+1)
    privacyDir=os.path.join(Utils.ConfigDir, "privacy1")
    nodeCert = os.path.join(privacyDir, "{}.crt".format(participantName))
    nodeKey = os.path.join(privacyDir, "{}_key.pem".format(participantName))
    return "--p2p-tls-own-certificate-file {} --p2p-tls-private-key-file {}".format(nodeCert, nodeKey)

def makeMultiCAArgs(index):
    privacy1Dir=os.path.join(Utils.ConfigDir, "privacy1")
    privacy2Dir=os.path.join(Utils.ConfigDir, "privacy2")
    multiCAFilePath = os.path.join(Utils.ConfigDir, "multiCA_cert.pem")
    with open(os.path.join(privacy1Dir, "CA_cert.pem"), "r") as f1:
        with open(os.path.join(privacy2Dir, "CA_cert.pem"), "r") as f2:
            with open(multiCAFilePath, "w") as multiCAFile:
                multiCAFile.write(f1.read())
                multiCAFile.write(f2.read())
    
    participantName = Node.participantName(index+1)
    nodeCert = os.path.join(privacy1Dir, "{}.crt".format(participantName))
    nodeKey = os.path.join(privacy1Dir, "{}_key.pem".format(participantName))
    return "--p2p-tls-own-certificate-file {} --p2p-tls-private-key-file {} --p2p-tls-security-group-ca-file {}".format(nodeCert, nodeKey, multiCAFilePath)


###############################################################
# privacy_tls_network
#
# General test for TLS peer to peer connections with different certificates or without certificates at all
#
###############################################################

Print=Utils.Print

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v"})

pnodes=1 # always one as we testing just connection between peers so we don't need bios setup and need just eosio producer
totalNodes=11 # 6 valid and 5 invalid cases, exclusing bios
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
Utils.Debug=args.v

testSuccessful=False
cluster=Cluster(host=TestHelper.LOCAL_HOST, port=TestHelper.DEFAULT_PORT, walletd=True)
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=True, cleanup=True)

    # this is for producer and 1 valid node
    Cluster.generateCertificates("privacy1", 2)
    # those are certificates for invalid cases
    Cluster.generateCertificates("privacy2", 2)

    specificExtraNodeosArgs = {}

    # VALID CASES
    # producer node
    specificExtraNodeosArgs[-1] = Cluster.getPrivacyArguments("privacy1", 0)
    # valid network member
    specificExtraNodeosArgs[0] = Cluster.getPrivacyArguments("privacy1", 1)
    # testing duplicate
    specificExtraNodeosArgs[1] = Cluster.getPrivacyArguments("privacy1", 1)
    # valid root certificate used as participant
    specificExtraNodeosArgs[2] = makeRootCertArgs("privacy1")
    # no CA certificate
    specificExtraNodeosArgs[3] = makeNoCAArgs(1)
    # CA files contains multiple certificates
    specificExtraNodeosArgs[4] = makeMultiCAArgs(1)
    # certificate from privacy2 group. identical to INVALID case index #6
    # what makes it VALID is connection to multi CA node
    specificExtraNodeosArgs[5] = Cluster.getPrivacyArguments("privacy2", 0)

    # INVALID CASES
    # certificate out of group with same name #1
    specificExtraNodeosArgs[6] = Cluster.getPrivacyArguments("privacy2", 0)
    # certificate out of group with same name #2
    specificExtraNodeosArgs[7] = Cluster.getPrivacyArguments("privacy2", 1)
    # using invalid self-signed certificate
    specificExtraNodeosArgs[8] = makeRootCertArgs("privacy2")
    # valid CA and certificate but invalid password
    specificExtraNodeosArgs[9] = makeWrongPrivateKeyArgs(1)
    # no TLS arguments at all
    specificExtraNodeosArgs[10] = ""

    topo = { 0 : [1,2,3,4,6,7,8,9,10],
             1 : [0,2,3,4,5,6,7,8,9,10],
             2 : [0,1,3,4,5,6,7,8,9,10],
             3 : [0,1,2,4,5,6,8,9,10],      # node 3 doesn't have CA certificate and will allow connection from node 9 with wrong private key because of it won't verify it at all
                                            # node 9 doesn't verify certificate so connection with wrong private key here becomes legal case
             4 : [0,1,2,3,5,9,10],          # node 4 is not linked to 6,7,8 as due to multicertificate CA it will send blocks there
                                            # in cases 6,7,8 we want to test different CA scenarios so no connection to node 4
             5 : [4],                       
             6 : [0,1,2,3,7,8,9,10],        # no connection to 4,5 due to multi CA test
             7 : [0,1,2,3,6,8,9,10],        # no connection to 4,5 due to multi CA test
             8 : [0,1,2,6,7,9,10],          # no connection to node 4,5 due to multi CA test
             9 : [0,1,2,4,5,6,7,8,10],      # no connection to node 3
             10: [0,1,2,3,4,5,6,7,8,9]}
    if not cluster.launch(pnodes=pnodes, totalNodes=totalNodes, dontBootstrap=True, configSecurityGroup=False, specificExtraNodeosArgs=specificExtraNodeosArgs, topo=topo, printInfo=True):
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    
    cluster.biosNode.waitForLibToAdvance()

    validNodes = [cluster.getNode(x) for x in range(5)] + [cluster.biosNode]
    cluster.verifyInSync(specificNodes=validNodes)
    if Utils.Debug:
        Print("*****************************")
        Print("          Valid nodes        ")
        Print("*****************************")
        cluster.reportInfo(validNodes)

    invalidNodes = [cluster.getNode(x) for x in range(6, totalNodes)]
    for node in invalidNodes:
        assert node.getHeadBlockNum() == 1, (Print("node {} receives blocks while it shouldn't".format(node.nodeId)), cluster.reportInfo([node]))
    if Utils.Debug:
        Print("*****************************")
        Print("          Invalid nodes      ")
        Print("*****************************")
        cluster.reportInfo(invalidNodes)

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, True, True, keepLogs, True, dumpErrorDetails)