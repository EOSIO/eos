#!/bin/bash
docker volume create --name=nodeos-data-volume
docker volume create --name=keosd-data-volume
if [ ! -h ~/keosd-data-volume ]; then ln -s ~/nodeos-data-volume ~/keosd-data-volume; fi
docker-compose -f docker-compose.yml up -d
printf "\nPlease ensure that you place your genesis.json file, if necessary, into ~/nodeos-data-volume, then restart the container\n"
# Ensure nodeos is started
sleep 5
docker start nodeos
