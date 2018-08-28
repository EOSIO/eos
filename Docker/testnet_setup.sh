#!/bin/bash

docker volume create --name=nodeosd-test-data-volume
docker volume create --name=keosd-test-data-volume
docker-compose -f docker-compose-testnet.yml up -d
# v1.1.0 issue?
docker start nodeos_test
