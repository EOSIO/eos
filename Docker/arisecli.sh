#!/bin/bash

# Usage:
# Go into cmd loop: sudo ./arisecli.sh
# Run single cmd:  sudo ./arisecli.sh <arisecli paramers>

PREFIX="docker-compose exec aosd arisecli"
if [ -z $1 ] ; then
  while :
  do
    read -e -p "arisecli " cmd
    history -s "$cmd"
    $PREFIX $cmd
  done
else
  $PREFIX "$@"
fi
