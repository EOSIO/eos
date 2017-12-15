#!/bin/bash

path=`dirname $0`

# Sync transaction across cluster with 1 producing, 1 non-producing nodes
scriptNames[0]='trans_sync_across_mixed_cluster_test.sh -p 1 -n 2'
# Transfer funds across multiple accounts in cluster and validate
scriptNames[1]='distributed-transactions-test.py -p 1 -n 5'

for scriptName in "${scriptNames[@]}"; do
    echo "********* Running script: $scriptName"
    $path/$scriptName
    echo "********* Script run finished"
done

echo "********* END"
