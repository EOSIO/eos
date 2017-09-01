#!/bin/bash
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
