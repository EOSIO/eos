#!/bin/bash

path=`dirname $0`

# Sync transaction across cluster with 1 producing, 1 non-producing nodes
scriptNames[0]='trans_sync_across_mixed_cluster_test.sh -p 1 -n 2'

for scriptName in "${scriptNames[@]}"; do
    echo "********* Running script: $scriptName"
    $path/$scriptName
    echo "********* Script run finished"
done

echo "********* END"
