#!/bin/bash

ARCH=$( uname )
pgrep_opts="-fl"
if [ "$ARCH" == "Linux" ]; then
    OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )

    if [[ $OS_NAME == "Ubuntu" || $OS_NAME == "Linux Mint" || $OS_NAME == "Fedora" || $OS_NAME == "CentOS Linux" || $OS_NAME == "arch" ]]; then
        pgrep_opts="-a"
    fi
fi

fun_ret=0

getChildCount () {
    name=$1

    #echo child name: $name
    cmd="pgrep $pgrep_opts $name | wc -l"
    echo CMD: $cmd
    count=`pgrep $pgrep_opts $name | wc -l`
    fun_ret=$count
}

# launch nodeos
cmd="programs/eosio-launcher/eosio-launcher -p 1 -n 1 -s mesh -d 1 -f  --nodeos '--max-transaction-time 5000 --filter-on *'"
echo CMD: $cmd
programs/eosio-launcher/eosio-launcher -p 1 -n 1 -s mesh -d 1 -f  --nodeos '--max-transaction-time 5000 --filter-on *'

name="nodeos"

getChildCount $name
count=$fun_ret
echo child count 1: $count

sleep 1

getChildCount $name
count=$fun_ret
echo child count 2: $count

sleep 1

getChildCount $name
count=$fun_ret
echo child count 3: $count

# kill nodeos
cmd="pkill -9 $name"
echo CMD: $cmd
pkill -9 $name

sleep 1

getChildCount $name
count=$fun_ret
echo child count 4: $count

sleep 1

getChildCount $name
count=$fun_ret
echo child count 5: $count

if [[ $count -ne 0 ]]; then
    echo ERROR: child process is still alive.
    ps aux
    exit 1
fi

echo Child process is terminated. Test successful

exit 0
