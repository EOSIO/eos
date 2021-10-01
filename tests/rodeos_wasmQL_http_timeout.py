#!/usr/bin/env python3

from testUtils import Utils
from TestHelper import TestHelper
from rodeos_utils import RodeosCluster
from TestHelper import AppArgs
import time
import os

try: 
    import requests
except ImportError:
    os.system('pip install requests')
    import requests

try: 
    import requests_unixsocket
except ImportError:
    os.system('pip install requests_unixsocket')
    import requests_unixsocket

###############################################################
# rodeos_wasmQL http time out test
# 
#   This test verifies timeout for wasmQL http connection through TCP/IP and unix socket works as expected. This test
#   launches a cluster of 1 SHiP and 2 rodeos instances and verifies that the cluster is stable and rodeos's are receiving blocks.
#   
#
###############################################################

Print=Utils.Print

extraArgs=AppArgs()
extraArgs.add_bool("--eos-vm-oc-enable", "Use OC for rodeos")
extraArgs.add_bool("--load-test-enable", "Enable load test")

args=TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"}, extraArgs)
enableOC=args.eos_vm_oc_enable
enableLoadTest=args.load_test_enable
Utils.Debug=args.v

TestHelper.printSystemInfo("BEGIN")
testSuccessful=False
def launch_cluster(num_ships, num_rodeos, unix_socket_option, timeout, eos_vm_oc_enable=False):
    with RodeosCluster(args.dump_error_details,
            args.keep_logs,
            args.leave_running,
            args.clean_run, unix_socket_option,
            'test.filter', './tests/test_filter.wasm',
            eos_vm_oc_enable,
            num_rodeos,
            num_ships,
            timeout) as cluster:
        maincwd=os.getcwd()
        Print("Testing cluster of {} ship nodes and {} rodeos nodes connecting through {}"\
            .format(num_ships, num_rodeos, (lambda x: 'Unix Socket' if (x==True) else 'TCP')(unix_socket_option)))
        for i in range(num_rodeos):
            assert cluster.waitRodeosReady(i), "Rodeos failed to stand up for a cluster of {} ship node and {} rodeos node".format(num_ships, num_rodeos)

        rodeosId = 0 # making http connection to rodeosId = 0

        if unix_socket_option:
            s = requests_unixsocket.Session()
            os.chdir(cluster.rodeosDir[rodeosId])
            r = s.get('http+unix://rodeos{}.sock/v1/chain/get_info'.format(rodeosId))
        else:
            s = requests.Session()
            r = s.get('{}v1/chain/get_info'.format(cluster.wqlEndPoints[rodeosId]))
        
        assert r.status_code == 200, "Http request was not successful"

        time.sleep(timeout // 1000) # idle connection for timeout

        try:
            if unix_socket_option:
                s.get('http+unix://rodeos{}.sock/v1/chain/get_info'.format(rodeosId))
            else:
                s.get('{}v1/chain/get_info'.format(cluster.wqlEndPoints[rodeosId]))
        except requests.exceptions.RequestException:
            # connection timeout exception to happen here.
            pass
        else:
            if os.getcwd() != maincwd:
                os.chdir(maincwd)
            print('###### Error: WasmQL http connection timeout failed for {} ######'\
                .format((lambda x: 'unix-socket' if (x==True) else 'TCP')(unix_socket_option)))
            exit(1)
            
        if os.getcwd() != maincwd:
            os.chdir(maincwd)
        cluster.setTestSuccessful(True)

# Test cases: (1 ships, 2 rodeos)
numSHiPs=[1]
numRodeos=[2]
NumTestCase=len(numSHiPs)
for i in [True, False]: # True means Unix-socket option, False means TCP/IP
    for j in range(NumTestCase):
        launch_cluster(numSHiPs[j], numRodeos[j], i, 5000, enableOC)

testSuccessful=True
errorCode = 0 if testSuccessful else 1
exit(errorCode)
