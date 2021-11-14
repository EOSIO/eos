#!/usr/bin/env python3

from testUtils import Utils
from TestHelper import TestHelper
from rodeos_utils import RodeosCluster
from TestHelper import AppArgs
import time
import os
import logging
import requests
import requests_unixsocket


###############################################################
# rodeos_wasmQL http time out test
# 
#   This test verifies timeout for wasmQL http connection through TCP/IP and unix socket to rodeos. The scenario in this test
#   is such that a persistent http connection is made to rodeos with timeout period of 5 second. Then we make 10 queries with a 
#   delay of 2.5s between each query and the session must be alive as the connection was active within the timeout period. 
#   Then the connection is kept in idle for the timeout period and rodeos must close out this idle session. Once we make another query
#   to rodeos, request module must reset the dropped session and throws this action in its logs. Through this log, this test can verify that
#   rodeos already closed out the idle session and new persistent connection has to be made again.
###############################################################

Print=Utils.Print

extraArgs=AppArgs()
extraArgs.add_bool("--eos-vm-oc-enable", "Use OC for rodeos")
extraArgs.add_bool("--unix-socket", "Enable unix socket")

args=TestHelper.parse_args({"--dump-error-details","--keep-logs","-v","--leave-running","--clean-run"}, extraArgs)
enableOC=args.eos_vm_oc_enable
enableUnixSocket=args.unix_socket
Utils.Debug=args.v

class LogStream(object): # this class parse log outputs from request module
    def __init__(self):
        self.logs = []

    def write(self, str):
        if len(self.logs) > 0 and 'Resetting dropped connection' in self.logs[-1]: # Once persistence conmection is closed
            self.logs[-1]+= str                                                    # request throws 'Resetting dropped connection' in logs
        else:
            self.logs.append(str)

    def flush(self):
        pass

    def getlastlog(self):
        if len(self.logs) > 0:
            return self.logs[-1]
        return 'No log record'


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
        log_stream = LogStream()
        logging.basicConfig(stream=log_stream, level=logging.DEBUG, format="%(message)s")

        if unix_socket_option:
            s = requests_unixsocket.Session()  # Opening persistence connetion
            os.chdir(cluster.rodeosDir[rodeosId])
            for i in range(10):                # making 10 wasm queries to rodeos with a half timeout delay netween each query
                r = s.get('http+unix://rodeos{}.sock/v1/chain/get_info'.format(rodeosId))
                time.sleep(timeout // 2000)
                assert r.status_code == 200, "Http request was not successful" # Http request must be successful
                print(log_stream.getlastlog())
                assert 'Resetting dropped connection: localhost' not in log_stream.getlastlog() # Connection made with timeout period so connection must be alive
        else:
            s = requests.Session()
            for i in range(10):
                r = s.get('{}v1/chain/get_info'.format(cluster.wqlEndPoints[rodeosId]))
                time.sleep(timeout // 2000)
                assert r.status_code == 200, "Http request was not successful"
                print(log_stream.getlastlog())
                assert 'Resetting dropped connection: localhost' not in log_stream.getlastlog()

        time.sleep(timeout // 1000) # idle connection for timeout period and expect rodeos to close the http session
                                    # should see a timeout warning in rodeos logs also
        if unix_socket_option:
            s.get('http+unix://rodeos{}.sock/v1/chain/get_info'.format(rodeosId)) # making new query 
            assert r.status_code == 200, "Http request was not successful" # Http request must be successful
            print(log_stream.getlastlog())
        else: 
            s.get('{}v1/chain/get_info'.format(cluster.wqlEndPoints[rodeosId]))
            assert r.status_code == 200, "Http request was not successful" # Http request must be successful
            print(log_stream.getlastlog())
        assert 'Resetting dropped connection' in log_stream.getlastlog() # Rodeos have already closed out previous session due to timeout
                                                                         # so request module has to open up a new session and throws 
                                                                         # Resetting dropped connection in its logs 
        if os.getcwd() != maincwd:
            os.chdir(maincwd)
        cluster.setTestSuccessful(True)

# Test cases: (1 ships, 2 rodeos)
numSHiPs=[1]
numRodeos=[2]

launch_cluster(numSHiPs[0], numRodeos[0], enableUnixSocket, 5000, enableOC)

testSuccessful=True
errorCode = 0 if testSuccessful else 1
exit(errorCode)
