#!/bin/bash

pnodes=10
npnodes=0
topo=star
if [ -n "$1" ]; then
    pnodes=$1
    if [ -n "$2" ]; then
        topo=$2
        if [ -n "$3" ]; then
            npnodes=$3
        fi
    fi
fi

total_nodes=`expr $pnodes + $npnodes`

rm -rf tn_data_*
programs/launcher/launcher -p $pnodes -n $total_nodes -s $topo
sleep 7
echo "start" > test.out
port=8888
endport=`expr $port + $total_nodes`
echo endport = $endport
while [ $port  -ne $endport ]; do
    programs/eosc/eosc --port $port get block 1 >> test.out 2>&1;
    port=`expr $port + 1`
done

grep 'producer"' test.out | tee summary | sort -u -k2 | tee unique
prodsfound=`wc -l < unique`
lines=`wc -l < summary`
if [ $lines -eq $total_nodes -a $prodsfound -eq 1 ]; then
    echo all synced
    programs/launcher/launcher -k 15
    exit
fi
echo $lines reports out of $total_nodes and prods = $prodsfound
sleep 18
programs/eosc/eosc --port 8888 get block 5
sleep 15
programs/eosc/eosc --port 8888 get block 10
sleep 15
programs/eosc/eosc --port 8888 get block 15
sleep 15
programs/eosc/eosc --port 8888 get block 20
sleep 15
echo "pass 2" > test.out
port=8888
while [ $port  -ne $endport ]; do
    programs/eosc/eosc --port $port get block 1 >> test.out 2>&1;
    port=`expr $port + 1`
done


grep 'producer"' test.out | tee summary | sort -u -k2 | tee unique
prodsfound=`wc -l < unique`
lines=`wc -l < summary`
if [ $lines -eq $total_nodes -a $prodsfound -eq 1 ]; then
    echo all synced
    programs/launcher/launcher -k 15
    exit
fi
echo ERROR: $lines reports out of $total_nodes and prods = $prodsfound
exit 1
