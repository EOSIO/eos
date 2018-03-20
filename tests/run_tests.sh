#!/bin/bash

path=`dirname $0`
disable_slow_tests=0
if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    echo "Disabling slow tests."
    disable_slow_tests=1
fi

# Test to sync transaction across cluster with 1 producing, 1 non-producing nodes
scriptNames[0]='trans_sync_across_mixed_cluster_test.sh -p 1 -n 2'

# Test to transfer funds across multiple accounts in cluster and validate
scriptNames[1]='distributed-transactions-test.py -p 1 -n 4 --not-noon --dump-error-detail'

if [[ $disable_slow_tests == 0 ]]; then
    # Test to transfer funds across multiple accounts in cluster and validate
    scriptNames[2]='distributed-transactions-remote-test.py --not-noon --dump-error-detail'

    # Test to kill node with SIGKILL and restart with argument "--resync-blockchain"
    scriptNames[3]='restart-scenarios-test.py -c resync -p4 --not-noon --dumpErrorDetails'

    # Test to kill node with SIGKILL and restart with argument "--replay-blockchain"
    scriptNames[4]='restart-scenarios-test.py -c replay -p4 --not-noon --dumpErrorDetails'

    # Test for validating consensus based block production. We introduce malicious producers which
    #  reject all transactions.
    scriptNames[5]='consensusValidationMaliciousProducers.py --not-noon -w 80 --dumpErrorDetails'
fi

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
