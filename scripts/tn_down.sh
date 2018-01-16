#!/bin/bash
#
# tn_bounce is used to restart a node that is acting badly or is down.
# usage: tn_bounce.sh [arglist]
# arglist will be passed to the node's command line. First with no modifiers
# then with --replay and then a third time with --resync
#
# the data directory and log file are set by this script. Do not pass them on
# the command line.
#
# in most cases, simply running ./tn_bounce.sh is sufficient.
#


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
        kill -15 $runtest
    else
        pkill -15 $runtest
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
        kill -9 $runtest
    else
        pkill -9 $runtest
    fi
fi
