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
    __walletLogFile="test_keosd_output.log"
    __walletDataDir="test_wallet_0"

    # pylint: disable=too-many-arguments
    # walletd [True|False] True=Launch wallet(keosd) process; False=Manage launch process externally.
    def __init__(self, walletd, nodeosPort=8888, nodeosHost="localhost", port=9899, host="localhost"):
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

        cmd="%s --data-dir %s --config-dir %s --http-server-address=%s:%d --verbose-http-errors" % (
            Utils.EosWalletPath, WalletMgr.__walletDataDir, WalletMgr.__walletDataDir, self.host, self.port)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        with open(WalletMgr.__walletLogFile, 'w') as sout, open(WalletMgr.__walletLogFile, 'w') as serr:
            popen=subprocess.Popen(cmd.split(), stdout=sout, stderr=serr)
            self.__walletPid=popen.pid

        # Give keosd time to warm up
        time.sleep(2)
        return True

    def create(self, name, accounts=None, exitOnError=True):
        wallet=self.wallets.get(name)
        if wallet is not None:
            if Utils.Debug: Utils.Print("Wallet \"%s\" already exists. Returning same." % name)
            return wallet
        p = re.compile(r'\n\"(\w+)\"\n', re.MULTILINE)
        cmdDesc="wallet create"
        cmd="%s %s %s --name %s --to-console" % (Utils.EosClientPath, self.endpointArgs, cmdDesc, name)
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
                    if Utils.Debug: Utils.Print("%s was not accepted, delaying for %d seconds and trying again" % (cmdDesc, delay))
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
            Utils.EosClientPath, self.endpointArgs, wallet.name, account.ownerPrivateKey)
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
                Utils.EosClientPath, self.endpointArgs, wallet.name, account.activePrivateKey)
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
        cmd="%s %s wallet private_keys --name %s --password %s " % (Utils.EosClientPath, self.endpointArgs, wallet.name, wallet.password)
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
            Utils.Print("Contents of %s:" % (WalletMgr.__walletLogFile))
            Utils.Print("=================================================================")
            with open(WalletMgr.__walletLogFile, "r") as f:
                shutil.copyfileobj(f, sys.stdout)

    def killall(self, allInstances=False):
        """Kill keos instances. allInstances will kill all keos instances running on the system."""
        if self.__walletPid:
            Utils.Print("Killing wallet manager process %d" % (self.__walletPid))
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
