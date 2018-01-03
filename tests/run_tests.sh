#!/bin/bash

path=`dirname $0`

# Test to sync transaction across cluster with 1 producing, 1 non-producing nodes
scriptNames[0]='trans_sync_across_mixed_cluster_test.sh -p 1 -n 2'

# Test to transfer funds across multiple accounts in cluster and validate
scriptNames[1]='distributed-transactions-test.py -p 1 -n 4'

# Test to kill node with SIGKILL and restart with argument "--resync-blockchain"
scriptNames[2]='restart-scenarios-test.py -c resync -p3 --dumpErrorDetails'

# Test to kill node with SIGKILL and restart with argument "--replay-blockchain"
scriptNames[3]='restart-scenarios-test.py -c replay -p3 --dumpErrorDetails'

# Test to kill node with SIGKILL and restart without an explicit chain sync strategy argument
#  This doesn't seem to be deterministic and ocassionally fails, so disabling it.
# scriptNames[4]='restart-scenarios-test.py -c none -p3 --dumpErrorDetails'

failingTestsCount=0
for scriptName in "${scriptNames[@]}"; do
    echo "********* Running script: $scriptName"
    $path/$scriptName
    retCode=$?
    if [[ $retCode != 0 ]]; then
	failingTests[$failingTestsCount]=$scriptName
	(( failingTestsCount++ ))
    fi
    echo "********* Script run finished"
done

if [[ $failingTestsCount > 0 ]]; then
    echo "********* END ALL TESTS WITH FAILURES" >&2
    echo "********* FAILED TESTS ($failingTestsCount): " >&2
    printf '*********\t%s\n' "${failingTests[@]}" >&2
else
    echo "********* END ALL TESTS SUCCESSFULLY"
fi
exit $failingTestsCount
