import testUtils
import p2p_test_peers
import random

RemoteCmd = p2p_test_peers.P2PTestPeers

class LossyNetwork:
    prefix="tc qdisc add dev eth0 root netem loss"
    cmds=["0%", "1%", "10%", "50%", "90%", "99%"]

    def maxIndex(self):
        return len(self.cmds)

    def execute(self, cmdInd, node, testerAccount, eosio):
        print("\n==== lossy network test: set loss ratio into %s ====" % (self.cmds[cmdInd]))
        RemoteCmd.exec("ssh", self.prefix + " " + self.cmds[cmdInd])
        s=""
        for i in range(12):
            s=s+random.choice("abcdefghijklmnopqrstuvwxyz12345")
        testerAccount.name = s
        transIdlist = []
        print("==== creating account %s ====" % (s))
        trans = node.createAccount(testerAccount, eosio, stakedDeposit=0, waitForTransBlock=True)
        transIdlist.append(node.getTransId(trans))
        return transIdlist
    
    def on_exit(self):
        RemoteCmd.exec("ssh", self.prefix + " " + self.cmds[0])