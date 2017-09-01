#!/bin/bash

# Usage:
# Go into cmd loop: sudo ./eosc.sh
# Run single cmd:  sudo ./eosc.sh <eosc paramers>

PREFIX="docker exec docker_eos_1 eosc"
if [ -z $1 ] ; then
  while :
  do
    read -e -p "eosc " cmd
    history -s "$cmd"
    $PREFIX $cmd
  done
else
  $PREFIX $@
fi
