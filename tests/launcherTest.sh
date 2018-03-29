#!/bin/bash

pnodes=1
total_nodes=1
topo=maze
delay=0

cmd="programs/eosio-launcher/eosio-launcher -p $pnodes -n $total_nodes -d $delay -f --verbose"
echo cmd: $cmd
eval $cmd

cmd="sleep 5"
echo cmd: $cmd
eval $cmd

cmd="ps -ef | grep nodeos"
echo cmd: $cmd
eval $cmd

for file in "var/lib/node_00/stdout.txt" "var/lib/node_bios/stdout.txt" "bios_boot.sh" "setprods.json" "etc/eosio/node_00/config.ini" "etc/eosio/node_bios/config.ini" "etc/eosio/node_00/genesis.json" "etc/eosio/node_bios/genesis.json":
do
    echo File: $file
    cat $file
done

cmd="programs/eosio-launcher/eosio-launcher -k 15"
echo cmd: $cmd
eval $cmd

rm -rf staging
rm -rf var/lib/node_*
rm -rf etc/eosio/node_*
exit 1
