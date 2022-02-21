import re
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
from datetime import datetime
from sys import stdout
from sys import exit
import traceback

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

###########################################################################################

class ReturnType(EnumType):
    pass

addEnum(ReturnType, "raw")
addEnum(ReturnType, "json")

###########################################################################################

class BlockLogAction(EnumType):
    pass

addEnum(BlockLogAction, "make_index")
addEnum(BlockLogAction, "trim")
addEnum(BlockLogAction, "smoke_test")
addEnum(BlockLogAction, "return_blocks")
addEnum(BlockLogAction, "prune_transactions")
addEnum(BlockLogAction, "fix_irreversible_blocks")

###########################################################################################

class WaitSpec:

    def __init__(self, value, leeway=None):
        self.toCalculate = True if value == -1 else False
        if value is not None:
            assert isinstance(value, (int))
            assert value >= -1
        self.value = value
        self.leeway = leeway if leeway is not None else WaitSpec.default_leeway

    def __str__(self):
        append = "[calculated based on block production]" if self.toCalculate else ""
        desc = None
        if self.value is None:
            desc = "defaulted"
        elif self.value >= 0:
            desc = "%d sec" % (self.value)
        else:
            desc = ""
        return "WaitSpec timeout %s%s" % (desc, append)

    def convert(self, startBlockNum, endBlockNum):
        if self.value is None or self.value != -1:
            return

        timeout = self.leeway
        if (endBlockNum > startBlockNum):
            # calculation is performing worst case (irreversible block progression) which at worst will waste 5 seconds
            blocksPerWindow = 12
            blockWindowsToWait = (endBlockNum - startBlockNum + blocksPerWindow - 1) / blocksPerWindow
            secondsPerWindow = blocksPerWindow / 2
            timeout += blockWindowsToWait * secondsPerWindow

        self.value = timeout

    def asSeconds(self):
        assert self.value != -1, "Called method with WaitSpec for calculating the appropriate timeout (WaitSpec.convert)," +\
                                 " but convert method was never called. This means that either one of the methods the WaitSpec" +\
                                 " is passed to needs to call convert, or else WaitSpec.calculate(...) should not have been passed."
        retVal = self.value if self.value is not None else WaitSpec.default_seconds
        return retVal

    @staticmethod
    def calculate(leeway=None):
        return WaitSpec(value=-1, leeway=leeway)

    @staticmethod
    def default(leeway=None):
        return WaitSpec(value=None, leeway=leeway)

    default_seconds = 60
    default_leeway = 10

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
    ShuttingDown=False

    EosBlockLogPath="programs/eosio-blocklog/eosio-blocklog"

    FileDivider="================================================================="
    DataRoot="var"
    DataDir="%s/lib/" % (DataRoot)
    ConfigDir="etc/eosio/"

    TimeFmt='%Y-%m-%dT%H:%M:%S.%f'

    @staticmethod
    def timestamp():
        return datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S.%f")

    @staticmethod
    def checkOutputFileWrite(time, cmd, output, error):
        stop=Utils.timestamp()
        if not hasattr(Utils, "checkOutputFile"):
            if not os.path.isdir(Utils.DataRoot):
                if Utils.Debug: Utils.Print("creating dir %s in dir: %s" % (Utils.DataRoot, os.getcwd()))
                os.mkdir(Utils.DataRoot)
            filename="%s/subprocess_results.log" % (Utils.DataRoot)
            if Utils.Debug: Utils.Print("opening %s in dir: %s" % (filename, os.getcwd()))
            Utils.checkOutputFile=open(filename,"w")

        Utils.checkOutputFile.write(Utils.FileDivider + "\n")
        Utils.checkOutputFile.write("start={%s}\n" % (time))
        Utils.checkOutputFile.write("cmd={%s}\n" % (" ".join(cmd)))
        Utils.checkOutputFile.write("cout={%s}\n" % (output))
        Utils.checkOutputFile.write("cerr={%s}\n" % (error))
        Utils.checkOutputFile.write("stop={%s}\n" % (stop))

    @staticmethod
    def Print(*args, **kwargs):
        stackDepth=len(inspect.stack())-2
        s=' '*stackDepth
        stdout.write(Utils.timestamp() + " ")
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
    def getDateString(dt):
        return "%d_%02d_%02d_%02d_%02d_%02d" % (
            dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second)

    @staticmethod
    def nodeExtensionToName(ext):
        r"""Convert node extension (bios, 0, 1, etc) to node name. """
        prefix="node_"
        if ext == "bios":
            return prefix + ext

        return "node_%02d" % (ext)

    @staticmethod
    def getNodeDataDir(ext, relativeDir=None, trailingSlash=False):
        path=os.path.join(Utils.DataDir, Utils.nodeExtensionToName(ext))
        if relativeDir is not None:
           path=os.path.join(path, relativeDir)
        if trailingSlash:
           path=os.path.join(path, "")
        return path

    @staticmethod
    def getNodeConfigDir(ext, relativeDir=None, trailingSlash=False):
        path=os.path.join(Utils.ConfigDir, Utils.nodeExtensionToName(ext))
        if relativeDir is not None:
           path=os.path.join(path, relativeDir)
        if trailingSlash:
           path=os.path.join(path, "")
        return path

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
        popen = Utils.delayedCheckOutput(cmd)
        return Utils.checkDelayedOutput(popen, cmd, ignoreError=ignoreError)

    @staticmethod
    def delayedCheckOutput(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE):
        if (isinstance(cmd, list)):
            popen=subprocess.Popen(cmd, stdout=stdout, stderr=stderr)
        else:
            popen=subprocess.Popen(cmd, stdout=stdout, stderr=stderr, shell=True)
        return popen

    @staticmethod
    def checkDelayedOutput(popen, cmd, ignoreError=False):
        assert isinstance(popen, subprocess.Popen)
        assert isinstance(cmd, (str,list))
        start=Utils.timestamp()
        (output,error)=popen.communicate()
        Utils.checkOutputFileWrite(start, cmd, output, error)
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
    def waitForTruth(lam, timeout=None, sleepTime=3, reporter=None):
        if timeout is None:
            timeout=WaitSpec.default()
        if isinstance(timeout, WaitSpec):
            timeout = timeout.asSeconds()

        currentTime=time.time()
        endTime=currentTime+timeout
        needsNewLine=False
        failReturnVal=None
        try:
            while endTime > currentTime:
                ret=lam()
                if ret:
                    return ret
                # save this to return the not Truth state for the passed in method
                failReturnVal=ret
                remaining = endTime - currentTime
                if sleepTime > remaining:
                    sleepTime = remaining
                if Utils.Debug:
                    Utils.Print("cmd: sleep %d seconds, remaining time: %d seconds" %
                                (sleepTime, remaining))
                else:
                    stdout.write('.')
                    stdout.flush()
                    needsNewLine=True
                if reporter is not None:
                    reporter()
                time.sleep(sleepTime)
                currentTime=time.time()
        finally:
            if needsNewLine:
                Utils.Print()

        return failReturnVal


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
    def runCmdReturnStr(cmd, trace=False, silentErrors=False):
        cmdArr=shlex.split(cmd)
        return Utils.runCmdArrReturnStr(cmdArr, trace=trace, silentErrors=silentErrors)

    @staticmethod
    def runCmdArrReturnStr(cmdArr, trace=False, silentErrors=False):
        retStr=Utils.checkOutput(cmdArr,ignoreError=silentErrors)
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
        # pylint: disable=deprecated-method
        # pgrep differs on different platform (amazonlinux1 and 2 for example). We need to check if pgrep -h has -a available and add that if so:
        try:
            pgrepHelp = re.search('-a', subprocess.Popen("pgrep --help 2>/dev/null", shell=True, stdout=subprocess.PIPE).stdout.read().decode('utf-8'))
            pgrepHelp.group(0) # group() errors if -a is not found, so we don't need to do anything else special here.
            pgrepOpts="-a"
        except AttributeError as error:
            # If no -a, AttributeError: 'NoneType' object has no attribute 'group'
            pgrepOpts="-fl"

        return "pgrep %s %s" % (pgrepOpts, serverName)

    @staticmethod
    def getBlockLog(nodeDataDir, blockLogAction=BlockLogAction.return_blocks, outputFile=None, first=None, last=None, extraArgs="", throwException=False, silentErrors=False, exitOnError=False):
        blockLogLocation = os.path.join(nodeDataDir, "blocks")
        assert(isinstance(blockLogLocation, str))
        outputFileStr=" --output-file %s " % (outputFile) if outputFile is not None else ""
        firstStr=" --first %s " % (first) if first is not None else ""
        lastStr = " --last %s " % (last) if last is not None else ""

        blockLogActionStr=None
        returnType=ReturnType.raw
        if blockLogAction==BlockLogAction.return_blocks:
            blockLogActionStr=""
            returnType=ReturnType.json
        elif blockLogAction==BlockLogAction.make_index:
            blockLogActionStr=" --make-index "
        elif blockLogAction==BlockLogAction.trim:
            blockLogActionStr=" --trim "
        elif blockLogAction==BlockLogAction.fix_irreversible_blocks:
            blockLogActionStr=" --fix-irreversible-blocks "
        elif blockLogAction==BlockLogAction.smoke_test:
            blockLogActionStr = " --smoke-test "
        elif blockLogAction == BlockLogAction.prune_transactions:
            blockLogActionStr = " --state-history-dir {}/state-history --prune-transactions ".format(nodeDataDir)
        else:
            unhandledEnumType(blockLogAction)

        cmd="%s --blocks-dir %s --as-json-array %s%s%s%s %s" % (Utils.EosBlockLogPath, blockLogLocation, outputFileStr, firstStr, lastStr, blockLogActionStr, extraArgs)
        if Utils.Debug: Utils.Print("cmd: %s" % (cmd))
        rtn=None
        try:
            if returnType==ReturnType.json:
                rtn=Utils.runCmdReturnJson(cmd, silentErrors=silentErrors)
            else:
                rtn=Utils.runCmdReturnStr(cmd, silentErrors=silentErrors)
        except subprocess.CalledProcessError as ex:
            if throwException:
                raise
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

    @staticmethod
    def addAmount(assetStr: str, deltaStr: str) -> str:
        asset = assetStr.split()
        if len(asset) != 2:
            return None
        delta = deltaStr.split()
        if len(delta) != 2:
            return None
        if asset[1] != delta[1]:
            return None
        return "{0} {1}".format(round(float(asset[0]) + float(delta[0]), 4), asset[1])

    @staticmethod
    def deduceAmount(assetStr: str, deltaStr: str) -> str:
        asset = assetStr.split()
        if len(asset) != 2:
            return None
        delta = deltaStr.split()
        if len(delta) != 2:
            return None
        if asset[1] != delta[1]:
            return None
        return "{0} {1}".format(round(float(asset[0]) - float(delta[0]), 4), asset[1])
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

