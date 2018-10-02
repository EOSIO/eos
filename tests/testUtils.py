import subprocess
import time
import os
from collections import deque
from collections import namedtuple
import inspect
import json
import shlex
from sys import stdout
from sys import exit
import traceback

###########################################################################################
class Utils:
    Debug=False
    FNull = open(os.devnull, 'w')

    EosClientPath="programs/cleos/cleos"

    EosWalletName="keosd"
    EosWalletPath="programs/keosd/"+ EosWalletName

    EosServerName="nodeos"
    EosServerPath="programs/nodeos/"+ EosServerName

    EosLauncherPath="programs/eosio-launcher/eosio-launcher"
    MongoPath="mongo"
    ShuttingDown=False
    CheckOutputDeque=deque(maxlen=10)

    @staticmethod
    def Print(*args, **kwargs):
        stackDepth=len(inspect.stack())-2
        s=' '*stackDepth
        stdout.write(s)
        print(*args, **kwargs)

    SyncStrategy=namedtuple("ChainSyncStrategy", "name id arg")

    SyncNoneTag="none"
    SyncReplayTag="replay"
    SyncResyncTag="resync"
    SyncHardReplayTag="hardReplay"

    SigKillTag="kill"
    SigTermTag="term"

    systemWaitTimeout=90
    irreversibleTimeout=60

    @staticmethod
    def setIrreversibleTimeout(timeout):
        Utils.irreversibleTimeout=timeout

    @staticmethod
    def setSystemWaitTimeout(timeout):
        Utils.systemWaitTimeout=timeout

    @staticmethod
    def getChainStrategies():
        chainSyncStrategies={}

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncNoneTag, 0, "")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncReplayTag, 1, "--replay-blockchain")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncResyncTag, 2, "--delete-all-blocks")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        chainSyncStrategy=Utils.SyncStrategy(Utils.SyncHardReplayTag, 3, "--hard-replay-blockchain")
        chainSyncStrategies[chainSyncStrategy.name]=chainSyncStrategy

        return chainSyncStrategies

    @staticmethod
    def checkOutput(cmd):
        assert(isinstance(cmd, list))
        popen=subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (output,error)=popen.communicate()
        Utils.CheckOutputDeque.append((output,error,cmd))
        if popen.returncode != 0:
            raise subprocess.CalledProcessError(returncode=popen.returncode, cmd=cmd, output=error)
        return output.decode("utf-8")

    @staticmethod
    def errorExit(msg="", raw=False, errorCode=1):
        if Utils.ShuttingDown:
            Utils.Print("ERROR:" if not raw else "", " errorExit called during shutdown, ignoring.  msg=", msg)
            return
        Utils.Print("ERROR:" if not raw else "", msg)
        traceback.print_stack(limit=-1)
        exit(errorCode)

    @staticmethod
    def cmdError(name, cmdCode=0):
        msg="FAILURE - %s%s" % (name, ("" if cmdCode == 0 else (" returned error code %d" % cmdCode)))
        Utils.Print(msg)

    @staticmethod
    def waitForObj(lam, timeout=None):
        if timeout is None:
            timeout=60

        endTime=time.time()+timeout
        needsNewLine=False
        try:
            while endTime > time.time():
                ret=lam()
                if ret is not None:
                    return ret
                sleepTime=3
                if Utils.Debug:
                    Utils.Print("cmd: sleep %d seconds, remaining time: %d seconds" %
                                (sleepTime, endTime - time.time()))
                else:
                    stdout.write('.')
                    stdout.flush()
                    needsNewLine=True
                time.sleep(sleepTime)
        finally:
            if needsNewLine:
                Utils.Print()

        return None

    @staticmethod
    def waitForBool(lam, timeout=None):
        myLam = lambda: True if lam() else None
        ret=Utils.waitForObj(myLam, timeout)
        return False if ret is None else ret

    @staticmethod
    def filterJsonObject(data):
        firstIdx=data.find('{')
        lastIdx=data.rfind('}')
        retStr=data[firstIdx:lastIdx+1]
        return retStr

    @staticmethod
    def runCmdArrReturnJson(cmdArr, trace=False, silentErrors=True):
        retStr=Utils.checkOutput(cmdArr)
        jStr=Utils.filterJsonObject(retStr)
        if trace: Utils.Print ("RAW > %s" % (retStr))
        if trace: Utils.Print ("JSON> %s" % (jStr))
        if not jStr:
            msg="Received empty JSON response"
            if not silentErrors:
                Utils.Print ("ERROR: "+ msg)
                Utils.Print ("RAW > %s" % retStr)
            raise TypeError(msg)

        try:
            jsonData=json.loads(jStr)
            return jsonData
        except json.decoder.JSONDecodeError as ex:
            Utils.Print (ex)
            Utils.Print ("RAW > %s" % retStr)
            Utils.Print ("JSON> %s" % jStr)
            raise

    @staticmethod
    def runCmdReturnStr(cmd, trace=False):
        retStr=Utils.checkOutput(cmd.split())
        if trace: Utils.Print ("RAW > %s" % (retStr))
        return retStr

    @staticmethod
    def runCmdReturnJson(cmd, trace=False, silentErrors=False):
        cmdArr=shlex.split(cmd)
        return Utils.runCmdArrReturnJson(cmdArr, trace=trace, silentErrors=silentErrors)


###########################################################################################
class Account(object):
    # pylint: disable=too-few-public-methods

    def __init__(self, name):
        self.name=name

        self.ownerPrivateKey=None
        self.ownerPublicKey=None
        self.activePrivateKey=None
        self.activePublicKey=None


    def __str__(self):
        return "Name: %s" % (self.name)

###########################################################################################

def addEnum(enumClassType, type):
    setattr(enumClassType, type, enumClassType(type))

def unhandledEnumType(type):
    raise RuntimeError("No case defined for type=%s" % (type.type))

class EnumType:

    def __init__(self, type):
        self.type=type

    def __str__(self):
        return self.type
