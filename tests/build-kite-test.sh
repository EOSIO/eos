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
    cmd="pgrep $pgrep_opts $name | grep '\-s' | wc -l"
    echo CMD: $cmd
    count=`pgrep $pgrep_opts $name | grep '\-s' | wc -l`
    # pgrep $pgrep_opts $name | grep '\-s'
    fun_ret=$count
}

sleep_on=0
while getopts ":s" opt; do
  case ${opt} in
    s ) # process option a
    sleep_on=1
      ;;
    \? ) echo "Usage: cmd [-s]"
      ;;
  esac
done

if [[ $sleep_on == 1 ]]; then
    echo Child is heading to sleep...
    sleep 2000000
    exit 0
fi

me=`basename "$0"`
name=${me:0:-3}

# spawn child process
cmd="nohup $0 -s"
echo CMD: $cmd
nohup $0 -s&

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

cmd="pkill sleep"
echo CMD: $cmd
pkill sleep

sleep 1

getChildCount $name
count=$fun_ret
echo child count 4: $count

sleep 1

getChildCount $name
count=$fun_ret
echo child count 5: $count

if [[ $count != 0 ]]; then
    echo ERROR: child process is still alive.
    exit 1
fi

echo Child process is terminated. Test successful

cmd="jobs -p"
echo CMD: $cmd
jobs -p

exit 0
