#!/bin/bash

cd ../..
count=5
topo=star
if [ -n "$1" ]; then
    count=$1
    if [ -n "$2" ]; then
        topo=$2
    fi
fi

total_nodes=`expr $count + 2`

rm -rf tn_data_*
programs/launcher/launcher -p $count -n $total_nodes -s $topo
sleep 7
echo "start" > test.out
port=8888
endport=`expr $port + $count`
echo endport = $endport
while [ $port  -ne $endport ]; do
    programs/eosc/eosc --port $port get block 1 >> test.out 2>&1;
    port=`expr $port + 1`
done

grep 'producer"' test.out | tee summary | sort -u -k2 | tee unique
prods=`wc -l < unique`
lines=`wc -l < summary`
if [ $lines -eq $count -a $prods -eq 1 ]; then
    echo all synced
    programs/launcher/launcher -k 15
    cd -
    exit
fi
echo lines = $lines and prods = $prods
sleep 18
programs/eosc/eosc --port 8888 get block 5
sleep 18
programs/eosc/eosc --port 8888 get block 10
sleep 16
programs/eosc/eosc --port 8888 get block 15
sleep 16
programs/eosc/eosc --port 8888 get block 20
sleep 16
echo "pass 2" > test.out
port=8888
while [ $port  -ne $endport ]; do
    programs/eosc/eosc --port $port get block 1 >> test.out 2>&1;
    port=`expr $port + 1`
done

grep 'producer"' test.out

programs/launcher/launcher -k 9
cd -
