import subprocess
import time
import shutil
import signal
import os
from collections import namedtuple
import re
import sys

from testUtils import Utils

Wallet=namedtuple("Wallet", "name password host port")
# pylint: disable=too-many-instance-attributes
class WalletMgr(object):
    __walletLogOutFile=""
    __walletLogErrFile=""
    __walletDataDir=""

    # pylint: disable=too-many-arguments
    # walletd [True|False] True=Launch wallet(keosd) process; False=Manage launch process externally.
    def __init__(self, walletd, nodeosPort=-1, nodeosHost="localhost", port=-1, host="localhost"):
        self.walletd=walletd
        self.nodeosPort=nodeosPort
        self.nodeosHost=nodeosHost
        self.port=port
        self.host=host
        self.wallets={}
        self.__walletPid=None

        WalletMgr.__walletDataDir=("cluster%05d/test_wallet" % Utils.clusterID)

        try:
            Utils.Print("WalletMgr: creating directory %s" % (WalletMgr.__walletDataDir))
            os.mkdir(WalletMgr.__walletDataDir)
        except:
            pass

        WalletMgr.__walletLogOutFile = ("cluster%05d/test_keosd_out.log" % Utils.clusterID)
        WalletMgr.__walletLogErrFile = ("cluster%05d/test_keosd_err.log" % Utils.clusterID)
        
        if self.port == -1:
            self.port = Utils.portBase + Utils.clusterID * Utils.cluster_stride + Utils.node_stride + Utils.port_type_keosd     
        if self.nodeosPort == -1:
            self.nodeosPort = Utils.portBase + Utils.clusterID * Utils.cluster_stride + Utils.node_stride + Utils.port_type_http

    def getWalletEndpointArgs(self):
        if not self.walletd or not self.isLaunched():
            return ""

        return " --wallet-url http://%s:%d" % (self.host, self.port)

    def getArgs(self):
        return " --url http://%s:%d%s %s" % (self.nodeosHost, self.nodeosPort, self.getWalletEndpointArgs(), Utils.MiscEosClientArgs)

    def isLaunched(self):
        return self.__walletPid is not None

    def isLocal(self):
        return self.host=="localhost" or self.host=="127.0.0.1"

    def launch(self):
        if not self.walletd:
            Utils.Print("ERROR: Wallet Manager wasn't configured to launch keosd")
            return False

        if self.isLaunched():
            return True

        tries = 30
        while not Utils.arePortsAvailable(self.port):
            if tries == 0:
                Utils.Print("failed to launch walletMgr because port %d not available" % (self.port, Utils.EosWalletPath))
                return False
            Utils.Print("wallet port %d not available for %s, wait..." % (self.port, Utils.EosWalletPath))
            tries = tries - 1
            time.sleep(2)

        pgrepCmd=Utils.pgrepCmd(WalletMgr.__walletDataDir)
        if Utils.Debug:
            psOut=Utils.checkOutput(pgrepCmd.split(), ignoreError=True)
            if psOut:
                statusMsg=""
                if psOut:
                    statusMsg+=" %s - {%s}." % (pgrepCmd, psOut)
                Utils.Print("Launching %s, note similar processes running. %s" % (Utils.EosWalletName, statusMsg))

        cmd="%s --data-dir %s --config-dir %s --http-server-address=%s:%d --verbose-http-errors" % (
            Utils.EosWalletPath, WalletMgr.__walletDataDir, WalletMgr.__walletDataDir, self.host, self.port)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        with open(WalletMgr.__walletLogOutFile, 'w') as sout, open(WalletMgr.__walletLogErrFile, 'w') as serr:
            popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
            self.__walletPid=popen.pid

        # Give keosd time to warm up
        time.sleep(2)

        try:
            if Utils.Debug: Utils.Print("Checking if %s launched. %s" % (Utils.EosWalletName, pgrepCmd))
            psOut=Utils.checkOutput(pgrepCmd.split())
            Utils.Print("walletMgr launched successfully for cluster %d. output: %s" % (Utils.clusterID, psOut))
        except subprocess.CalledProcessError as ex:
            Utils.errorExit("Failed to launch the wallet manager. failed cmd is %s" % (pgrepCmd))

        return True

    def create(self, name, accounts=None, exitOnError=True):
        wallet=self.wallets.get(name)
        if wallet is not None:
            if Utils.Debug: Utils.Print("Wallet \"%s\" already exists. Returning same." % name)
            return wallet
        p = re.compile(r'\n\"(\w+)\"\n', re.MULTILINE)
        cmdDesc="wallet create"
        cmd="%s %s %s --name %s --to-console" % (Utils.EosClientPath, self.getArgs(), cmdDesc, name)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        retStr=None
        maxRetryCount=4
        retryCount=0
        while True:
            try:
                retStr=Utils.checkOutput(cmd.split())
                break
            except subprocess.CalledProcessError as ex:
                retryCount+=1
                if retryCount<maxRetryCount:
                    delay=10
                    pgrepCmd=Utils.pgrepCmd(Utils.EosWalletName)
                    psOut=Utils.checkOutput(pgrepCmd.split())
                    portStatus="N/A"
                    if self.isLocal():
                        if Utils.arePortsAvailable(self.port):
                            portStatus="AVAILABLE"
                        else:
                            portStatus="NOT AVAILABLE"
                    if Utils.Debug: Utils.Print("%s was not accepted, delaying for %d seconds and trying again. port %d is %s. %s - {%s}" % (cmdDesc, delay, self.port, pgrepCmd, psOut))
                    time.sleep(delay)
                    continue

                msg=ex.output.decode("utf-8")
                errorMsg="ERROR: Failed to create wallet - %s. %s" % (name, msg)
                if exitOnError:
                    Utils.errorExit("%s" % (errorMsg))
                Utils.Print("%s" % (errorMsg))
                return None

        m=p.search(retStr)
        if m is None:
            if exitOnError:
                Utils.cmdError("could not create wallet %s" % (name))
                Utils.errorExit("Failed  to create wallet %s" % (name))

            Utils.Print("ERROR: wallet password parser failure")
            return None
        p=m.group(1)
        wallet=Wallet(name, p, self.host, self.port)
        self.wallets[name] = wallet

        if accounts:
            self.importKeys(accounts,wallet)

        return wallet

    def importKeys(self, accounts, wallet, ignoreDupKeyWarning=False):
        for account in accounts:
            Utils.Print("Importing keys for account %s into wallet %s." % (account.name, wallet.name))
            if not self.importKey(account, wallet, ignoreDupKeyWarning):
                Utils.Print("ERROR: Failed to import key for account %s" % (account.name))
                return False

    def importKey(self, account, wallet, ignoreDupKeyWarning=False):
        warningMsg="Key already in wallet"
        cmd="%s %s wallet import --name %s --private-key %s" % (
            Utils.EosClientPath, self.getArgs(), wallet.name, account.ownerPrivateKey)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        try:
            Utils.checkOutput(cmd.split())
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            if warningMsg in msg:
                if not ignoreDupKeyWarning:
                    Utils.Print("WARNING: This key is already imported into the wallet.")
            else:
                Utils.Print("ERROR: Failed to import account owner key %s. %s" % (account.ownerPrivateKey, msg))
                return False

        if account.activePrivateKey is None:
            Utils.Print("WARNING: Active private key is not defined for account \"%s\"" % (account.name))
        else:
            cmd="%s %s wallet import --name %s  --private-key %s" % (
                Utils.EosClientPath, self.getArgs(), wallet.name, account.activePrivateKey)
            if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
            try:
                Utils.checkOutput(cmd.split())
            except subprocess.CalledProcessError as ex:
                msg=ex.output.decode("utf-8")
                if warningMsg in msg:
                    if not ignoreDupKeyWarning:
                        Utils.Print("WARNING: This key is already imported into the wallet.")
                else:
                    Utils.Print("ERROR: Failed to import account active key %s. %s" %
                                (account.activePrivateKey, msg))
                    return False

        return True

    def lockWallet(self, wallet):
        cmd="%s %s wallet lock --name %s" % (Utils.EosClientPath, self.getArgs(), wallet.name)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            Utils.Print("ERROR: Failed to lock wallet %s." % (wallet.name))
            return False

        return True

    def unlockWallet(self, wallet):
        cmd="%s %s wallet unlock --name %s" % (Utils.EosClientPath, self.getArgs(), wallet.name)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        popen=subprocess.Popen(cmd.split(), stdout=Utils.FNull, stdin=subprocess.PIPE)
        _, errs = popen.communicate(input=wallet.password.encode("utf-8"))
        if 0 != popen.wait():
            Utils.Print("ERROR: Failed to unlock wallet %s: %s" % (wallet.name, errs.decode("utf-8")))
            return False

        return True

    def lockAllWallets(self):
        cmd="%s %s wallet lock_all" % (Utils.EosClientPath, self.getArgs())
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
            Utils.Print("ERROR: Failed to lock all wallets.")
            return False

        return True

    def getOpenWallets(self):
        wallets=[]

        p = re.compile(r'\s+\"(\w+)\s\*\",?\n', re.MULTILINE)
        cmd="%s %s wallet list" % (Utils.EosClientPath, self.getArgs())
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        retStr=None
        try:
            retStr=Utils.checkOutput(cmd.split())
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Failed to open wallets. %s" % (msg))
            return False

        m=p.findall(retStr)
        if m is None:
            Utils.Print("ERROR: wallet list parser failure")
            return None
        wallets=m

        return wallets

    def getKeys(self, wallet):
        keys=[]

        p = re.compile(r'\n\s+\"(\w+)\"\n', re.MULTILINE)
        cmd="%s %s wallet private_keys --name %s --password %s " % (Utils.EosClientPath, self.getArgs(), wallet.name, wallet.password)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        retStr=None
        try:
            retStr=Utils.checkOutput(cmd.split())
        except subprocess.CalledProcessError as ex:
            msg=ex.output.decode("utf-8")
            Utils.Print("ERROR: Failed to get keys. %s" % (msg))
            return False
        m=p.findall(retStr)
        if m is None:
            Utils.Print("ERROR: wallet private_keys parser failure")
            return None
        keys=m

        return keys


    def dumpErrorDetails(self):
        Utils.Print("=================================================================")
        if self.__walletPid is not None:
            Utils.Print("Contents of %s:" % (WalletMgr.__walletLogOutFile))
            Utils.Print("=================================================================")
            with open(WalletMgr.__walletLogOutFile, "r") as f:
                shutil.copyfileobj(f, sys.stdout)
            Utils.Print("Contents of %s:" % (WalletMgr.__walletLogErrFile))
            Utils.Print("=================================================================")
            with open(WalletMgr.__walletLogErrFile, "r") as f:
                shutil.copyfileobj(f, sys.stdout)

    def killall(self, allInstances=False):
        """Kill keos instances. allInstances will kill all keos instances running on the system."""
        if self.__walletPid:
            Utils.Print("Killing wallet manager process %d" % (self.__walletPid))
            os.kill(self.__walletPid, signal.SIGKILL)

        if allInstances:
            cmd="pkill -9 %s" % (Utils.EosWalletName)
            Utils.Print("Killing all wallet instances, cmd: %s" % (cmd))
            subprocess.call(cmd.split())
        else:
            cmd=Utils.pgrepCmd(WalletMgr.__walletDataDir)
            psOut=Utils.checkOutput(cmd.split(), ignoreError=True)
            if psOut is not None:
                m=psOut.split()
                if len(m) > 0:
                    cmd="kill -9 %s" % (m[0])
                    if 0 != subprocess.call(cmd.split(), stdout=Utils.FNull):
                        if not silent: Utils.Print("Failed to kill wallet")                 

    @staticmethod
    def cleanup():
        Utils.Print("WalletMgr: removing dir %s" % (WalletMgr.__walletDataDir))
        dataDir=WalletMgr.__walletDataDir
        if os.path.isdir(dataDir) and os.path.exists(dataDir):
            shutil.rmtree(WalletMgr.__walletDataDir)
