import testUtils
import p2p_test_peers
import random
import time
import copy
import threading

class StressNetwork:
    speeds=[1,5,10,30,60,100,500]
    sec=10
    maxthreads=100
    trList=[]

    def maxIndex(self):
        return len(self.speeds)

    def randAcctName(self):
        s=""
        for i in range(12):
            s=s+random.choice("abcdefghijklmnopqrstuvwxyz12345")
        return s
    
    def _transfer(self, node, acc1, acc2, amount, threadId, round):
        memo="%d %d" % (threadId, round)
        tr = node.transferFunds(acc1, acc2, amount, memo)
        self.trList.append(tr)

    def execute(self, cmdInd, node, ta, eosio):
        print("\n==== network stress test: %d transaction(s)/s for %d secs ====" % (self.speeds[cmdInd], self.sec))
        total = self.speeds[cmdInd] * self.sec

        ta.name = self.randAcctName()
        acc1 = copy.copy(ta)
        print("creating new account %s" % (ta.name))
        tr = node.createAccount(ta, eosio, stakedDeposit=0, waitForTransBlock=False)
        print("transaction id %s" % (node.getTransId(tr)))

        ta.name = self.randAcctName()
        acc2 = copy.copy(ta)
        print("creating new account %s" % (ta.name))
        tr = node.createAccount(ta, eosio, stakedDeposit=0, waitForTransBlock=False)
        print("transaction id %s" % (node.getTransId(tr)))

        time.sleep(1.0)

        print("issue currency into %s" % (acc1.name))
        contract="eosio"
        action="issue"
        data="{\"to\":\"" + acc1.name + "\",\"quantity\":\"1000000.0000 EOS\"}"
        opts="--permission eosio@active"
        tr=node.pushMessage(contract, action, data, opts)
        print("transaction id %s" % (node.getTransId(tr[1])))

        time.sleep(1.0)

        self.trList = []
        expBal = 0
        nthreads=self.maxthreads
        if nthreads > self.speeds[cmdInd]:
            nthreads = self.speeds[cmdInd]
        cycle = int(total / nthreads)
        total = cycle * nthreads # rounding
        delay = 1.0 / self.speeds[cmdInd] * nthreads

        print("start currency trasfer from %s to %s for %d times with %d threads" % (acc1.name, acc2.name, total, nthreads))

        t00 = time.time()
        for k in range(cycle):
            t0 = time.time()
            amount = 1
            threadList = []
            for m in range(nthreads):
                th = threading.Thread(target = self._transfer,args = (node, acc1, acc2, amount, m, k))
                th.start()
                threadList.append(th)
            for m in range(len(threadList)):
                threadList[m].join()
            expBal = expBal + amount * nthreads
            t1 = time.time()
            if (t1-t0 < delay):
                time.sleep(delay - (t1-t0))
        t11 = time.time()
        print("time used = %lf" % (t11 - t00))

        actBal = node.getAccountBalance(acc2.name)
        print("account %s: expect Balance:%d, actual Balance %d" % (acc2.name, expBal, actBal))

        transIdlist = []
        for k in range(len(self.trList)):
            transIdlist.append(node.getTransId(self.trList[k]))
        return (transIdlist, acc2.name, expBal)
    
    def on_exit(self):
        print("end of network stress tests")

