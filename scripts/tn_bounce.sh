#!/bin/bash
pkill -9 eosd
sleep 3
now=`date +'%Y_%m_%d_%H_%M_%S'`
DD=`ls -d tn_data_??`
log=$DD/stderr.$now.txt
nohup programs/eosd/eosd --resync --data-dir $DD > $DD/stdout.txt  2> $log &
echo $! > $DD/eosd.pid
rm $DD/stderr.txt
ln -s stderr.$now.txt $DD/stderr.txt
