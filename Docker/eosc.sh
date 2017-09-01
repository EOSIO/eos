#!/bin/bash
while :
do
read  -p "eosc " cmd
docker exec docker_eos_1 eosc $cmd
done
