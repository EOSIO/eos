#!/usr/bin/env python3

import json
import os
import shutil
import time
import unittest
import socket
import re
import subprocess
import datetime

from testUtils import Utils
#from testUtils import Account
#from TestHelper import TestHelper
#from Node import Node
#from WalletMgr import WalletMgr

def test_IPv6(self):
    """Test IPv6 feature of HTTP plugin."""
    
    host = "::1"
    node_id = 6
    port = 8866
    data_dir = Utils.getNodeDataDir(node_id)
    print(f"test_IPv6: creating data_dir '{data_dir}'")
    if os.path.exists(data_dir):
        shutil.rmtree(data_dir)
    os.makedirs(data_dir)
    
    nodeos_plugins = (" --plugin %s --plugin %s") % ("eosio::producer_plugin", "eosio::http_plugin")
    nodeos_flags = (" --data-dir=%s --http-validate-host=%s --verbose-http-errors --http-ipv6 1") % (data_dir, 0)
    start_nodeos_cmd = ("%s -e -p eosio %s %s ") % (Utils.EosServerPath, nodeos_plugins, nodeos_flags)

    dt = datetime.now()
    dateStr=Utils.getDateString(dt)
    stdoutFile="%s/stdout.%s.txt" % (data_dir, dateStr)
    stderrFile="%s/stderr.%s.txt" % (data_dir, dateStr)
    with open(stdoutFile, 'w') as sout, open(stderrFile, 'w') as serr:
        Utils.Print("cmd: %s" % (start_nodeos_cmd))
        popen=subprocess.Popen(start_nodeos_cmd.split(), stdout=sout, stderr=serr)
        
        if Utils.Debug: Utils.Print("start Node host=%s, port=%s, pid=%s, cmd=%s" % (self.host, self.port, self.pid, self.cmd))

    time.sleep(self.sleep_s)  # wait for nodeos to come up 

    
    flow_id = 0
    scope_info = 0
    addr = (host, port, flow_id, scope_info)
    api_call = "v1/chain/get_info"
    req = Utils.makeHTTPReqStr(host, str(port), api_call, "", isIPv6=True)
    print(f"test_IPv6: req= '{req}'")
    sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    sock.settimeout(5)
    try:
        sock.connect(addr)
    except Exception as e:
        print(f"test_IPv6: unable to connect to [{host}]:{port}")
        print(e)
        Utils.errorExit("Failed to connect to nodeos.")

    enc = "utf-8"
    sock.settimeout(3)
    maxMsgSize = 2048
    try:
        sock.send(bytes(req, enc))
        resp_data = Utils.readSocketDataStr(sock, maxMsgSize, enc)
        Utils.Print('test_IPv6: resp_data= \n', resp_data)
    except Exception as e:
        Utils.Print(e)
        Utils.errorExit("Failed to send/receive on socket")

    # extract response body 
    try:
        (hdr, resp_json) = re.split('\r\n\r\n', resp_data)
    except Exception as e:
        Utils.Print(e)
        Utils.errorExit("test_IPv6: Improper HTTP response(s)") 

    try:
        resp = json.loads(resp_json)
    except Exception as e:
        Utils.Print(e)
        Utils.errorExit("test_IPv6: Could not parse JSON response")
    
    self.assertIn('chain_id', resp)
    nodeos_ipv6.kill(9)

if __name__ == "__main__":
    unittest.main()
