#!/bin/bash
#
# eosio-tn_up is a helper script used to start a node that was previously stopped.
# It is not intended to be run stand-alone; it is a companion to the
# eosio-tn_bounce.sh and eosio-tn_roll.sh scripts.

connected="0"

rundir=programs/nodeos
prog=nodeos


if [ "$PWD" != "$EOSIO_HOME" ]; then
    echo $0 must only be run from $EOSIO_HOME
    exit -1
fi

if [ ! -e $rundir/$prog ]; then
    echo unable to locate binary for nodeos
    exit -1
fi

if [ -z "$EOSIO_NDOE" ]; then
    echo data directory not set
    exit -1
fi

relaunch() {
    nohup $rundir/$prog $* --data-dir $datadir --config-dir etc/eosio/node_$EOSIO_NODE > $datadir/stdout.txt  2>> $log &
    pid=$!

    echo $pid > $datadir/$prog.pid

    for (( a = 10; $a; a = $(($a - 1)) )); do
        echo checking viability pass $((11 - $a))
        sleep 2
        running=`ps -hp $pid`
        if [ -z "$running" ]; then
            break;
        fi
        connected=`grep -c "net_plugin.cpp:.*connection" $log`
        if [ "$connected" -ne 0 ]; then
            break;
        fi
    done
}

datadir=var/lib/node_$EOSIO_NODE
now=`date +'%Y_%m_%d_%H_%M_%S'`
log=$datadir/stderr.$now.txt
touch $log
rm $datadir/stderr.txt
ln -s $log $datadir/stderr.txt


if [ -z "$EOSIO_LEVEL" ]; then
    echo starting with no modifiers
    relaunch $prog $rundir $*
    if [ \( -z "$running" \) -o \( "$connected" -eq 0 \) ]; then
        EOSIO_LEVEL=replay
    fi
fi

if [ $EOSIO_LEVEL == replay ]; then
    echo starting with replay
    relaunch $prog $rundir $* --replay
    if [ \( -z "$running" \) -o \( "$connected" -eq 0 \) ]; then
        EOSIO_LEVEL=resync
    fi
fi
if [ "$EOSIO_LEVEL" == resync ]; then
    echo starting wih resync
    relaunch $* --resync
fi
