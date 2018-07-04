import testUtils
import p2p_test_peers
import random

RemoteCmd = p2p_test_peers.P2PTestPeers

class LossyNetwork:
    cmd="tc qdisc add dev {dev} root netem loss"
    args=["0%", "1%", "10%", "50%", "90%", "99%"]

    resetcmd="tc qdisc del dev {dev} root"

    def maxIndex(self):
        return len(self.args)

    def execute(self, cmdInd, node, testerAccount, eosio):
        print("\n==== lossy network test: set loss ratio to %s ====" % (self.args[cmdInd]))
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
