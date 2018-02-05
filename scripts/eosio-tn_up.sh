#!/bin/bash
#
# eosio-tn_up is a helper script used to start a node that was previously stopped.
# It is not intended to be run stand-alone; it is a companion to the
# eosio-tn_bounce.sh and eosio-tn_roll.sh scripts.

connected="0"

relaunch() {
    prog=$1; shift
    DD=$1; shift
    RD=$1; shift

    now=`date +'%Y_%m_%d_%H_%M_%S'`
    log=$DD/stderr.$now.txt

    nohup $RD/$prog $* --data-dir $DD > $DD/stdout.txt  2> $log &
    pid=$!

    echo $pid > $DD/$prog.pid
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

if [ "$PWD" != "$EOSIO_HOME" ]; then
    echo $0 must only be run from $EOSIO_HOME
    exit -1
fi

prog=""
RD=""
for p in eosd eosiod; do
    prog=$p
    RD=bin
    if [ -f $RD/$prog ]; then
        break;
    else
        RD=programs/$prog
        if [ -f $RD/$prog ]; then
            break;
        fi
    fi
    prog=""
    RD=""
done

if [ \( -z "$prog" \) -o \( -z "$RD" \) ]; then
    echo unable to locate binary for eosd or eosiod
    exit 1
fi

if [ -z "$EOSIO_TN_RESTART_DATA_DIR" ]; then
    echo data directory not set
    exit 1
fi

DD=$EOSIO_TN_RESTART_DATA_DIR;

echo starting with no modifiers
relaunch $prog $DD $RD $*
if [ \( -z "$running" \) -o \( "$connected" -eq 0 \) ]; then
    echo base relaunch failed, trying with replay
    relaunch $prog $DD $RD $* --replay
    if [ \( -z "$running" \) -o \( "$connected" -eq 0 \) ]; then
        echo relaunch replay failed, trying with resync
        relaunch $prog $DD $RD $* --resync
    fi
fi

rm $DD/stderr.txt
ln -s stderr.$now.txt $DD/stderr.txt
