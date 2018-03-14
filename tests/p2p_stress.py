import testUtils
import p2p_test_peers
import random
import time

class StressNetwork:
    speeds=[1,5,10,30,60,100,500]
    sec=5

    def maxIndex(self):
        return len(self.speeds)

    def execute(self, cmdInd, node, ta, eosio):
        print("\n==== stress network test: %d transaction(s)/s for %d secs ====" % (self.speeds[cmdInd], self.sec))
        total = self.speeds[cmdInd] * self.sec
        acclist = []
        for k in range(total):
            s=""
            for i in range(10):
                s=s+random.choice("abcdefghijklmnopqrstuvwxyz12345")
            acclist.append(s)
        ta.name = s
        transIdlist = []
        print("==== creating %d account(s) ====" % (total))
        delay = 1.0 / self.speeds[cmdInd]
        t00 = time.time()
        t0 = time.time()
        trList = []
        for k in range(total):
            ta.name = acclist[k]
            tr = node.createAccount(ta, eosio, stakedDeposit=0, waitForTransBlock=False)
            trList.append(tr)
            t1 = time.time()
            if (t1-t0 < delay):
                time.sleep(delay - (t1-t0))
                t0 = time.time()
            else:
                t0 = t1
        t11 = time.time()
        print("time used = %lf" % (t11 - t00))
        for k in range(total):
            transIdlist.append(node.getTransId(trList[k]))
        return transIdlist
    
    def on_exit(self):
        print("end of stress network tests")

