#!/bin/bash

# Usage:
# Go into cmd loop: sudo ./enu-cli.sh
# Run single cmd:  sudo ./enu-cli.sh <enu-cli paramers>

PREFIX="docker exec docker_nodeos_1 enu-cli"
if [ -z $1 ] ; then
  while :
  do
    read -e -p "enu-cli " cmd
    history -s "$cmd"
    $PREFIX $cmd
  done
else
  $PREFIX $@
fi
