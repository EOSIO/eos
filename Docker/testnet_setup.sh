#!/bin/bash
docker volume create --name=nodeos-test-data-volume
docker volume create --name=keosd-test-data-volume
if [ ! -h ~/keosd-test-data-volume ]; then ln -s ~/nodeos-test-data-volume ~/keosd-test-data-volume; fi
docker-compose -f docker-compose-testnet.yml up -d
printf "\nPlease ensure that you place your genesis.json file, if necessary, into ~/nodeos-test-data-volume, then restart the container\n"
# Ensure nodeos is started
docker start nodeos_test
