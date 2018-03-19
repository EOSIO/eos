import testUtils
import p2p_test_peers
import random

RemoteCmd = p2p_test_peers.P2PTestPeers

class ImpairedNetwork:
    cmd="tc qdisc add dev {dev} root tbf limit 65536 burst 65536 rate"
    args=["1mbps","500kbps","100kbps","10kbps","1kbps"] #,"1bps"]

    resetcmd="tc qdisc del dev {dev} root"

    def maxIndex(self):
        return len(self.args)

    def execute(self, cmdInd, node, testerAccount, eosio):
        print("\n==== impaired network test: set network speed to %s ====" % (self.args[cmdInd]))
        RemoteCmd.exec(self.cmd + " " + self.args[cmdInd])
        s=""
        for i in range(12):
            s=s+random.choice("abcdefghijklmnopqrstuvwxyz12345")
        testerAccount.name = s
        transIdlist = []
        print("==== creating account %s ====" % (s))
        trans = node.createAccount(testerAccount, eosio, stakedDeposit=0, waitForTransBlock=True)
        if trans is None:
            return ([], "", 0.0, "failed to create account")
        transIdlist.append(node.getTransId(trans))
        return (transIdlist, "", 0.0, "")
    
    def on_exit(self):
        RemoteCmd.exec(self.resetcmd)
