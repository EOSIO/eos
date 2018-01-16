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

cd $EOSIO_HOME

prog=eosd
if [ ! \( -f programs/$prog/$prog \) ]; then
    prog=eosiod
fi

if [ -z "$EOSIO_TN_NODE" ]; then
    DD=`ls -d tn_data_??`
    ddcount=`echo $DD | wc -w`
    if [ $ddcount -ne 1 ]; then
        DD="all"
    fi
else
    DD=tn_data_$EOSIO_TN_NODE
    if [ ! \( -d $DD \) ]; then
        echo no directory named $PWD/$DD
        cd -
        exit 1
    fi
fi

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

SDIR=staging/eos
if [ -e $RD/$prog ]; then
    s1=`md5sum $RD/$prog`
    s2=`md5sum $SDIR/$prog`
    if [ "$s1" == "$s2" ]; then
        echo $HOSTNAME no update $SDIR/$p
        exit 1;
    fi
fi


echo DD = $DD

export EOSIO_TN_RESTART_DATA_DIR=$DD
bash $EOSIO_HOME/scripts/tn_down.sh

cp $SDIR/$RD/$prog $RD/$prog

bash $EOSIO_HOME/scripts/tn_up.sh $*
unset EOSIO_TN_RESTART_DATA_DIR

cd -
