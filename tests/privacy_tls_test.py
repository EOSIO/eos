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


###############################################################
# privacy_tls_network
#
# General test for TLS peer to peer connections with different certificates or without certificates at all
#
###############################################################

Print=Utils.Print

args = TestHelper.parse_args({"--dump-error-details","--keep-logs","-v"})

pnodes=1 #always one as we testing just connection between peers so we don't need bios setup and need just eosio producer
totalNodes=8 #3 valid and 5 invalid cases, exclusing bios
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
Utils.Debug=args.v

testSuccessful=False
cluster=Cluster(host=TestHelper.LOCAL_HOST, port=TestHelper.DEFAULT_PORT, walletd=True)
try:
    TestHelper.printSystemInfo("BEGIN")
    cluster.killall(allInstances=True, cleanup=True)

    #this is for producer and 1 valid node
    Cluster.generateCertificates("privacy1", 2)
    #those are certificates for invalid cases
    Cluster.generateCertificates("privacy2", 2)

    specificExtraNodeosArgs = {}

    #VALID CASES
    #producer node
    specificExtraNodeosArgs[-1] = Cluster.getPrivacyArguments("privacy1", 0)
    #valid network member
    specificExtraNodeosArgs[0] = Cluster.getPrivacyArguments("privacy1", 1)
    #testing duplicate
    specificExtraNodeosArgs[1] = Cluster.getPrivacyArguments("privacy1", 1)
    #valid root certificate used as participant
    specificExtraNodeosArgs[2] = makeRootCertArgs("privacy1")

    #INVALID CASES
    #certificate out of group with same name #1
    specificExtraNodeosArgs[3] = Cluster.getPrivacyArguments("privacy2", 0)
    #certificate out of group with same name #2
    specificExtraNodeosArgs[4] = Cluster.getPrivacyArguments("privacy2", 1)
    #using invalid self-signed certificate
    specificExtraNodeosArgs[5] = makeRootCertArgs("privacy2")
    #valid CA and certificate but invalid password
    specificExtraNodeosArgs[6] = makeWrongPrivateKeyArgs(1)
    #no TLS arguments at all
    specificExtraNodeosArgs[7] = ""

    if not cluster.launch(pnodes=pnodes, totalNodes=totalNodes, dontBootstrap=True, configSecurityGroup=False, specificExtraNodeosArgs=specificExtraNodeosArgs, printInfo=True):
        Utils.cmdError("launcher")
        Utils.errorExit("Failed to stand up eos cluster.")
    
    cluster.biosNode.waitForLibToAdvance()

    validNodes = [cluster.getNode(x) for x in range(3)] + [cluster.biosNode]
    cluster.verifyInSync(specificNodes=validNodes)
    if Utils.Debug:
        Print("*****************************")
        Print("          Valid nodes        ")
        Print("*****************************")
        cluster.reportInfo(validNodes)

    invalidNodes = [cluster.getNode(x) for x in range(3, totalNodes)]
    for node in invalidNodes:
        assert node.getHeadBlockNum() == 1
    if Utils.Debug:
        Print("*****************************")
        Print("          Invalid nodes      ")
        Print("*****************************")
        cluster.reportInfo(invalidNodes)

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, cluster.walletMgr, testSuccessful, True, True, keepLogs, True, dumpErrorDetails)