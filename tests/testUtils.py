import errno
import subprocess
import time
import os
import platform
from collections import deque
from collections import namedtuple
import inspect
import json
import shlex
import socket
from sys import stdout
from sys import exit
import traceback

###########################################################################################
class Utils:
    Debug=False
    FNull = open(os.devnull, 'w')

    EosClientPath="programs/cleos/cleos"
    MiscEosClientArgs="--no-auto-keosd"

    EosWalletName="keosd"
    EosWalletPath="programs/keosd/"+ EosWalletName

    EosServerName="nodeos"
    EosServerPath="programs/nodeos/"+ EosServerName

    EosLauncherPath="programs/eosio-launcher/eosio-launcher"
    MongoPath="mongo"
    ShuttingDown=False
    CheckOutputDeque=deque(maxlen=10)

    EosBlockLogPath="programs/eosio-blocklog/eosio-blocklog"

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
    def checkOutput(cmd, ignoreError=False):
        assert(isinstance(cmd, list))
        popen=subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (output,error)=popen.communicate()
        Utils.CheckOutputDeque.append((output,error,cmd))
        if popen.returncode != 0 and not ignoreError:
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
    def filterJsonObjectOrArray(data):
        firstObjIdx=data.find('{')
        lastObjIdx=data.rfind('}')
        firstArrayIdx=data.find('[')
        lastArrayIdx=data.rfind(']')
        if firstArrayIdx==-1 or lastArrayIdx==-1:
            retStr=data[firstObjIdx:lastObjIdx+1]
        elif firstObjIdx==-1 or lastObjIdx==-1:
            retStr=data[firstArrayIdx:lastArrayIdx+1]
        elif lastArrayIdx < lastObjIdx:
            retStr=data[firstObjIdx:lastObjIdx+1]
        else:
            retStr=data[firstArrayIdx:lastArrayIdx+1]
        return retStr

    @staticmethod
    def runCmdArrReturnJson(cmdArr, trace=False, silentErrors=True):
        retStr=Utils.checkOutput(cmdArr)
        jStr=Utils.filterJsonObjectOrArray(retStr)
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

    @staticmethod
    def arePortsAvailable(ports):
        """Check if specified port (as int) or ports (as set) is/are available for listening on."""
        assert(ports)
        if isinstance(ports, int):
            ports={ports}
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

    @staticmethod
    def pgrepCmd(serverName):
        pgrepOpts="-fl"
        # pylint: disable=deprecated-method
        if platform.linux_distribution()[0] in ["Ubuntu", "LinuxMint", "Fedora","CentOS Linux","arch"]:
            pgrepOpts="-a"

        return "pgrep %s %s" % (pgrepOpts, serverName)

    @staticmethod
    def getBlockLog(blockLogLocation, silentErrors=False, exitOnError=False):
        assert(isinstance(blockLogLocation, str))
        cmd="%s --blocks-dir %s --as-json-array" % (Utils.EosBlockLogPath, blockLogLocation)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        rtn=None
        try:
            rtn=Utils.runCmdReturnJson(cmd, silentErrors=silentErrors)
        except subprocess.CalledProcessError as ex:
            if not silentErrors:
                msg=ex.output.decode("utf-8")
                errorMsg="Exception during \"%s\". %s" % (cmd, msg)
                if exitOnError:
                    Utils.cmdError(errorMsg)
                    Utils.errorExit(errorMsg)
                else:
                    Utils.Print("ERROR: %s" % (errorMsg))
            return None

        if exitOnError and rtn is None:
            Utils.cmdError("could not \"%s\"" % (cmd))
            Utils.errorExit("Failed to \"%s\"" % (cmd))

        return rtn

    @staticmethod
    def compare(obj1,obj2,context):
        type1=type(obj1)
        type2=type(obj2)
        if type1!=type2:
            return "obj1(%s) and obj2(%s) are different types, so cannot be compared, context=%s" % (type1,type2,context)

        if obj1 is None and obj2 is None:
            return None

        typeName=type1.__name__
        if type1 == str or type1 == int or type1 == float or type1 == bool:
            if obj1!=obj2:
                return "obj1=%s and obj2=%s are different (type=%s), context=%s" % (obj1,obj2,typeName,context)
            return None

        if type1 == list:
            len1=len(obj1)
            len2=len(obj2)
            diffSizes=False
            minLen=len1
            if len1!=len2:
                diffSizes=True
                minLen=min([len1,len2])

            for i in range(minLen):
                nextContext=context + "[%d]" % (i)
                ret=Utils.compare(obj1[i],obj2[i], nextContext)
                if ret is not None:
                    return ret

            if diffSizes:
                return "left and right side %s comparison have different sizes %d != %d, context=%s" % (typeName,len1,len2,context)
            return None

        if type1 == dict:
            keys1=sorted(obj1.keys())
            keys2=sorted(obj2.keys())
            len1=len(keys1)
            len2=len(keys2)
            diffSizes=False
            minLen=len1
            if len1!=len2:
                diffSizes=True
                minLen=min([len1,len2])

            for i in range(minLen):
                key=keys1[i]
                nextContext=context + "[\"%s\"]" % (key)
                if key not in obj2:
                    return "right side does not contain key=%s (has %s) that left side does, context=%s" % (key,keys2,context)
                ret=Utils.compare(obj1[key],obj2[key], nextContext)
                if ret is not None:
                    return ret

            if diffSizes:
                return "left and right side %s comparison have different number of keys %d != %d, context=%s" % (typeName,len1,len2,context)

            return None

        return "comparison of %s type is not supported, context=%s" % (typeName,context)

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
