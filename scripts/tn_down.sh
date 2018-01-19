#!/bin/bash
#
# tn_down.sh is used by the tn_bounce.sh and tn_roll.sh scripts. It is intended to terminate
# specific EOS daemon processes.
#


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

if [ \( -z "$prog" \) -o \( -z "RD" \) ]; then
    echo unable to locate binary for eosd or eosiod
    exit 1
fi

if [ -z "$EOSIO_TN_RESTART_DATA_DIR" ]; then
    echo data directory not set
    exit 1
fi

DD=$EOSIO_TN_RESTART_DATA_DIR;
runtest=""
if [ $DD = "all" ]; then
    runtest=$prog
else
    runtest=`cat $DD/$prog.pid`
fi
echo runtest =  $runtest
running=`ps -e | grep $runtest | grep -cv grep `

if [ $running -ne 0 ]; then
    echo killing $prog

    if [ $runtest = $prog ]; then
        pkill -15 $runtest
    else
        kill -15 $runtest
    fi

    for (( a = 10; $a; a = $(($a - 1)) )); do
        echo waiting for safe termination, pass $((11 - $a))
        sleep 2
        running=`ps -e | grep $runtest | grep -cv grep`
        echo running = $running
        if [ $running -eq 0 ]; then
            break;
        fi
    done
fi

if [ $running -ne 0 ]; then
    echo killing $prog with SIGTERM failed, trying with SIGKILL
    if [ $runtest = $prog ]; then
        pkill -9 $runtest
    else
        kill -9 $runtest
    fi
fi
