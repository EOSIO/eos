#!/bin/bash
#
# tn_up is a helper script used to start a node that was previously stopped
# usage: tn_bounce.sh [ [arglist]
# arglist will be passed to the node's command line. First with no modifiers
# then with --replay and then a third time with --resync
#
# the data directory and log file are set by this script. Do not pass them on
# the command line.
#
# in most cases, simply running ./tn_bounce.sh is sufficient.
#

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

prog=""
RD=""
for p in eosd eosiod; do
    prog=$p
    RD=$EOSIO_HOME/bin
    if [ -f $RD/$prog ]; then
        break;
    else
        RD=$EOSIO_HOME/programs/$prog
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
