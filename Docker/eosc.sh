#!/bin/bash

# Usage:
# Go into cmd loop: sudo sh eosc.sh
# Run single cmd:  sudo sh eosc.sh <eosc paramers>

PREFIX="docker exec docker_eos_1 eosc"
if [ -z $1 ] ; then
  while :
  do
    read  -p "eosc " cmd
    $PREFIX $cmd
  done
else
  $PREFIX $@
fi
