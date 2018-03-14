import testUtils
import p2p_test_peers
import random

RemoteCmd = p2p_test_peers.P2PTestPeers

class ImpairedNetwork:
    prefix="tc qdisc add dev eth0 root tbf rate"
    cmds=["1mbps","500kbps","100kbps","10kbps","1kbps","1bps"]

    def maxIndex(self):
        return len(self.cmds)

    def execute(self, cmdInd, node, testerAccount, eosio):
        print("\n==== impaired network test: set network speed into %s ====" % (self.cmds[cmdInd]))
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

