#!/bin/bash

# Usage:
# Go into cmd loop: sudo ./eosioc.sh
# Run single cmd:  sudo ./eosioc.sh <eosioc paramers>

PREFIX="docker exec docker_eosiod_1 eosioc"
if [ -z $1 ] ; then
  while :
  do
    read -e -p "eosioc " cmd
    history -s "$cmd"
    $PREFIX $cmd
  done
else
  $PREFIX $@
fi
