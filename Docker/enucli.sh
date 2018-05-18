#!/bin/bash

# Usage:
# Go into cmd loop: sudo ./enucli.sh
# Run single cmd:  sudo ./enucli.sh <enucli paramers>

PREFIX="docker exec docker_enunode_1 enucli"
if [ -z $1 ] ; then
  while :
  do
    read -e -p "enucli " cmd
    history -s "$cmd"
    $PREFIX $cmd
  done
else
  $PREFIX $@
fi
