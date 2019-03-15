#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from Node import Node
from Node import ReturnType
from TestHelper import TestHelper

import decimal
import re
import time

###############################################################
# nodeos_run_test
# --dump-error-details <Upon error print etc/eosio/node_*/config.ini and var/lib/node_*/stderr.log to stdout>
# --keep-logs <Don't delete var/lib/node_* folders upon test completion>
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit
cmdError=Utils.cmdError
from core_symbol import CORE_SYMBOL

args = TestHelper.parse_args({"--host","--port","--prod-count","--defproducera_prvt_key","--defproducerb_prvt_key","--mongodb"
                              ,"--dump-error-details","--dont-launch","--keep-logs","-v","--leave-running","--only-bios","--clean-run"
                              ,"--sanity-test","--p2p-plugin","--wallet-port"})
server=args.host
port=args.port
debug=args.v
enableMongo=args.mongodb
defproduceraPrvtKey=args.defproducera_prvt_key
defproducerbPrvtKey=args.defproducerb_prvt_key
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
dontLaunch=args.dont_launch
dontKill=args.leave_running
prodCount=2 # args.prod_count
onlyBios=args.only_bios
killAll=args.clean_run
sanityTest=args.sanity_test
p2pPlugin=args.p2p_plugin
walletPort=args.wallet_port

Utils.Debug=debug
localTest=True
cluster=Cluster(host=server, port=port, walletd=True, enableMongo=enableMongo, defproduceraPrvtKey=defproduceraPrvtKey, defproducerbPrvtKey=defproducerbPrvtKey)
walletMgr=WalletMgr(True, port=walletPort)
testSuccessful=False
killEosInstances=not dontKill
killWallet=not dontKill
dontBootstrap=sanityTest

WalletdName=Utils.EosWalletName
ClientName="cleos"
timeout = .5 * 12 * 2 + 60 # time for finalization with 1 producer + 60 seconds padding
Utils.setIrreversibleTimeout(timeout)

try:
    TestHelper.printSystemInfo("BEGIN prod_preactivation_test.py")
    cluster.setWalletMgr(walletMgr)
    Print("SERVER: %s" % (server))
    Print("PORT: %d" % (port))

    if enableMongo and not cluster.isMongodDbRunning():
        errorExit("MongoDb doesn't seem to be running.")

    if localTest and not dontLaunch:
        cluster.killall(allInstances=killAll)
        cluster.cleanup()
        Print("Stand up cluster")
        if cluster.launch(pnodes=prodCount, totalNodes=prodCount, prodCount=prodCount, onlyBios=onlyBios, dontBootstrap=dontBootstrap, p2pPlugin=p2pPlugin, extraNodeosArgs=" --plugin eosio::producer_api_plugin") is False:
            cmdError("launcher")
            errorExit("Failed to stand up eos cluster.")

    Print("Validating system accounts after bootstrap")
    cluster.validateAccounts(None)

    node = cluster.getNode(0)
    cmd = "curl %s/v1/producer/get_supported_protocol_features" % (node.endpointHttp)
    Print("try to get supported feature list from Node 0 with cmd: %s" % (cmd))
    feature0=Utils.runCmdReturnJson(cmd)
    #Print("feature list:", feature0)

    node = cluster.getNode(1)
    cmd = "curl %s/v1/producer/get_supported_protocol_features" % (node.endpointHttp)
    Print("try to get supported feature list from Node 1 with cmd: %s" % (cmd))
    feature1=Utils.runCmdReturnJson(cmd)
    #Print("feature list:", feature1)

    if feature0 != feature1:
        errorExit("feature list mismatch between node 0 and node 1")
    else:
        Print("feature list from node 0 matches with that from node 1")
   
    if len(feature0) == 0:
        errorExit("No supported feature list")

    digest = ""
    for i in range(0, len(feature0)):
       feature = feature0[i]
       if feature["specification"][i]["value"] != "PREACTIVATE_FEATURE":
           continue
       else:
           digest = feature["feature_digest"]
   
    if len(digest) == 0:
        errorExit("code name PREACTIVATE_FEATURE not found") 

    Print("found digest ", digest, " of PREACTIVATE_FEATURE")

    node0 = cluster.getNode(0)
    contract="eosio.bios"
    contractDir="unittests/contracts/%s" % (contract)
    wasmFile="%s.wasm" % (contract)
    abiFile="%s.abi" % (contract)

    Print("publish a new bios contract %s should fails because env.is_feature_activated unresolveable" % (contractDir))
    retMap = node0.publishContract("eosio", contractDir, wasmFile, abiFile, True, shouldFail=True)

    #Print(retMap)
    if retMap["output"].decode("utf-8").find("unresolveable") < 0:
        errorExit("bios contract not result in expected unresolveable error")

    secwait = 60
    Print("Wait for defproducerb to produce...")
    node = cluster.getNode(1)
    while secwait > 0:
       info = node.getInfo()
       #Print("head producer:", info["head_block_producer"])
       if info["head_block_producer"] == "defproducerb": #defproducerb is in node0
          break
       time.sleep(1)
       secwait = secwait - 1
    
    if secwait <= 0:
       errorExit("No producer of defproducerb")
    
    cmd = "curl --data-binary '{\"protocol_features_to_activate\":[\"%s\"]}' %s/v1/producer/schedule_protocol_feature_activations" % (digest, node.endpointHttp)

    Print("try to preactivate feature on node 1, cmd: %s" % (cmd))
    result = Utils.runCmdReturnJson(cmd)

    if result["result"] != "ok":
        errorExit("failed to preactivate feature from producer plugin on node 1")
    else:
        Print("feature PREACTIVATE_FEATURE (%s) preactivation success" % (digest))

    time.sleep(2)
    Print("publish a new bios contract %s should fails because node1 is not producing block yet" % (contractDir))
    retMap = node0.publishContract("eosio", contractDir, wasmFile, abiFile, True, shouldFail=True)
    if retMap["output"].decode("utf-8").find("unresolveable") < 0:
        errorExit("bios contract not result in expected unresolveable error")

    Print("now wait for node 1 produce a block....(take some minutes)...")
    secwait = 480 # wait for node1 produce a block
    while secwait > 0:
       info = node.getInfo()
       #Print("head producer:", info["head_block_producer"])
       if info["head_block_producer"] >= "defproducerm" and info["head_block_producer"] <= "defproduceru":
          break
       time.sleep(1)
       secwait = secwait - 1

    if secwait <= 0:
       errorExit("No blocks produced by node 1")

    time.sleep(1)
    retMap = node0.publishContract("eosio", contractDir, wasmFile, abiFile, True)
    Print("sucessfully set new contract with new intrinsic!!!")

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful, killEosInstances, killWallet, keepLogs, killAll, dumpErrorDetails)

exit(0)
