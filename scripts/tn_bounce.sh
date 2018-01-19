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
        echo $HOSTNAME has $ddcount data directories, bounce not possible. Set environment variable
        echo EOSIO_TN_NODE to the 2-digit node id number to specify which node to bounce. For example:
        echo EOSIO_TN_NODE=06 $0 \<options\>
        cd -
        exit 1
    fi
else
    DD=tn_data_$EOSIO_TN_NODE
    if [ ! \( -d $DD \) ]; then
        echo no directory named $PWD/$DD
        cd -
        exit 1
    fi
fi

echo DD = $DD

export EOSIO_TN_RESTART_DATA_DIR=$DD
bash $EOSIO_HOME/scripts/tn_down.sh
bash $EOSIO_HOME/scripts/tn_up.sh $*
unset EOSIO_TN_RESTART_DATA_DIR

cd -
