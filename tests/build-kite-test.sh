#!/bin/bash

OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )
pgrep_opts="-fl"
#if platform.linux_distribution()[0] in ["Ubuntu", "LinuxMint", "Fedora","CentOS Linux","arch"]:
if [[ $OS_NAME == "Ubuntu" || $OS_NAME == "Linux Mint" || $OS_NAME == "Fedora" || $OS_NAME == "CentOS Linux" || $OS_NAME == "arch" ]]; then
   pgrep_opts="-a"
fi

# echo OS_NAME: $OS_NAME
# echo pgrep_opts: $pgrep_opts

fun_ret=0

getChildCount () {
    name=$1

    #echo child name: $name
    cmd="pgrep $pgrep_opts $name | grep '\-s' | wc -l"
    echo CMD: $cmd
    count=`pgrep $pgrep_opts $name | grep '\-s' | wc -l`
    fun_ret=$count
}

getLastChildPid() {
    name=$1

    #echo child name: $name
    cmd="pgrep $pgrep_opts $name | grep '\-s' | tail -n 1 | cut -f 1 -d ' '"
    echo CMD: $cmd
    pid=`pgrep $pgrep_opts $name | grep '\-s' | tail -n 1 | cut -f 1 -d ' '`
    fun_ret=$pid
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
    sleep 100000
    exit 0
fi

me=`basename "$0"`
name=${me:0:-3}

# spawn child process
cmd="nohup $0 -s"
echo CMD: $cmd
nohup $0 -s&

getLastChildPid $name
cPid=$fun_ret
echo Child pid: $cPid

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

cmd="kill -9 $pid"
echo CMD: $cmd
kill -9 $pid

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

echo Child process is termniated.
exit 0
