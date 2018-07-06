import testUtils
import p2p_test_peers
import random
import time
import copy
import threading

from core_symbol import CORE_SYMBOL

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
        tr = node.createAccount(ta, eosio, stakedDeposit=0, waitForTransBlock=True)
        trid = node.getTransId(tr)
        if trid is None:
            return ([], "", 0.0, "failed to create account")
        print("transaction id %s" % (trid))

        ta.name = self.randAcctName()
        acc2 = copy.copy(ta)
        print("creating new account %s" % (ta.name))
        tr = node.createAccount(ta, eosio, stakedDeposit=0, waitForTransBlock=True)
        trid = node.getTransId(tr)
        if trid is None:
            return ([], "", 0.0, "failed to create account")
        print("transaction id %s" % (trid))

        print("issue currency0000 into %s" % (acc1.name))
        contract="eosio"
        action="issue"
        data="{\"to\":\"" + acc1.name + "\",\"quantity\":\"1000000.0000 "+CORE_SYMBOL+"\"}"
        opts="--permission eosio@active"
        tr=node.pushMessage(contract, action, data, opts)
        trid = node.getTransId(tr[1])
        if trid is None:
            return ([], "", 0.0, "failed to issue currency0000")
        print("transaction id %s" % (trid))
        node.waitForTransInBlock(trid)

        self.trList = []
        expBal = 0
        nthreads=self.maxthreads
        if nthreads > self.speeds[cmdInd]:
            nthreads = self.speeds[cmdInd]
        cycle = int(total / nthreads)
        total = cycle * nthreads # rounding
        delay = 1.0 / self.speeds[cmdInd] * nthreads

        print("start currency0000 trasfer from %s to %s for %d times with %d threads" % (acc1.name, acc2.name, total, nthreads))

        t00 = time.time()
        for k in range(cycle):
            t0 = time.time()
            amount = 1
            threadList = []
            for m in range(nthreads):
                th = threading.Thread(target = self._transfer,args = (node, acc1, acc2, amount, m, k))
                th.start()
                threadList.append(th)
            for th in threadList:
                th.join()
            expBal = expBal + amount * nthreads
            t1 = time.time()
            if (t1-t0 < delay):
                time.sleep(delay - (t1-t0))
        t11 = time.time()
        print("time used = %lf" % (t11 - t00))

        actBal = node.getAccountBalance(acc2.name)
        print("account %s: expect Balance:%d, actual Balance %d" % (acc2.name, expBal, actBal))

        transIdlist = []
        for tr in self.trList:
            trid = node.getTransId(tr)
            transIdlist.append(trid)
            node.waitForTransInBlock(trid)
        return (transIdlist, acc2.name, expBal, "")
    
    def on_exit(self):
        print("end of network stress tests")

