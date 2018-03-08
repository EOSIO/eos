#!/bin/bash

###############################################################
# -p <producing nodes count>
# -n <total nodes>
# -s <topology>
# -d <delay between nodes startup>
###############################################################

pnodes=10
topo=star
delay=0

args=`getopt p:n:s:d: $*`
if [ $? == 0 ]; then

    set -- $args
    for i; do
        case "$i"
        in
            -p) pnodes=$2;
                shift; shift;;
            -n) total_nodes=$2;
                shift; shift;;
            -d) delay=$2;
                shift; shift;;
            -s) topo="$2";
                shift; shift;;
            --) shift;
                break;;
        esac
    done
else
    echo "huh we got err $?"
    if [ -n "$1" ]; then
        pnodes=$1
        if [ -n "$2" ]; then
            topo=$2
            if [ -n "$3" ]; then
		total_nodes=$3
            fi
        fi
    fi
fi

total_nodes="${total_nodes:-`echo $pnodes`}"

rm -rf tn_data_*
if [ "$delay" == 0 ]; then
    programs/launcher/launcher -p $pnodes -n $total_nodes -s $topo
else
    programs/launcher/launcher -p $pnodes -n $total_nodes -s $topo -d $delay
fi

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
programs/launcher/launcher -k 15

count=0
while [ $count -lt $total_nodes ]; do
    num=$(printf "%02s" $count)
    echo =================================================================
    echo Contents of tn_data_${num}/config.ini:
    cat tn_data_${num}/config.ini
    echo =================================================================
    echo Contents of tn_data_${num}/stderr.txt:
    cat tn_data_${num}/stderr.txt
    count=`expr $count + 1`
done

exit 1
