#!/bin/bash
# Create data volume if it doesn't exist
if [ ! -d ~/nodeos-test-data-volume ]; then mkdir ~/nodeos-test-data-volume; fi
# If genesis.json is missing, don't allow this to run (adding it later breaks the chain)
if [ ! -e ~/nodeos-test-data-volume/genesis.json ]; then
	printf "Please ensure that you place your genesis.json file into ~/nodeos-test-data-volume before running this script!\n\n"
	exit
fi
docker volume create --name=nodeos-test-data-volume
docker volume create --name=keosd-test-data-volume
if [ ! -h ~/keosd-test-data-volume ]; then ln -s ~/nodeos-test-data-volume ~/keosd-test-data-volume; fi
docker-compose -f docker-compose-testnet.yml up -d
# Ensure nodeos is started
docker start nodeos_test
