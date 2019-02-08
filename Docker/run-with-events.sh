#!/bin/bash

script_path=$(dirname $(readlink -f $0))

if [[ "$1" == "cleanup" ]]; then
    docker-compose -p cyberway -f $script_path/docker-compose-events.yml down || exit 1
    docker volume rm cyberway-mongodb-data
    docker volume rm cyberway-nodeos-data
    docker volume rm cyberway-queue
    exit 0
elif [[ "$1" == "up" ]]; then
    docker-compose -p cyberway -f $script_path/docker-compose-events.yml up -d || exit 1
    exit 0
elif [[ "$1" == "down" ]]; then
    docker-compose -p cyberway -f $script_path/docker-compose-events.yml down || exit 1
    exit 0
fi

docker volume create cyberway-mongodb-data || true
docker volume create cyberway-nodeos-data || true
docker volume create cyberway-queue || true

if [[ ( -z "$NATS_USER" ) || ( -z "$NATS_PASS" ) ]]; then
    if [[ ! -e .env ]]; then
       NATS_PASS=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 32 | head -n 1)
       NATS_USER=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 8 | head -n 1)
       echo "NATS_USER=$NATS_USER" >> .env
       echo "NATS_PASS=$NATS_PASS" >> .env
    fi 
fi

docker-compose -p cyberway -f $script_path/docker-compose-events.yml up -d
