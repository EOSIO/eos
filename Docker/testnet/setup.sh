#!/bin/bash
source .env
echo "[Docker Container Creation Started][${COMPOSE_PROJECT_NAME}]"
DATAVOLUME="${HOME}/${NODEOS_VOLUME_NAME}"
# Create data volume if it doesn't exist
if [ ! -d "${DATAVOLUME}" ]; then
        mkdir $DATAVOLUME
        echo "[Created ${DATAVOLUME}]"
fi
# If genesis.json is missing, don't allow this to run (adding it later breaks the chain)
if [ ! -e "${DATAVOLUME}/genesis.json" ]; then
        printf "Please ensure that you place your genesis.json file into ${DATAVOLUME} before running this script!\n\n"
        exit
fi
docker volume create --name=$NODEOS_VOLUME_NAME 1> /dev/null
docker volume create --name=$KEOSD_VOLUME_NAME 1> /dev/null
if [ ! -h "${HOME}/${KEOSD_VOLUME_NAME}" ]; then ln -s $DATAVOLUME $HOME/$KEOSD_VOLUME_NAME; fi
docker-compose up -d
# Ensure nodeos is started
docker start "${COMPOSE_PROJECT_NAME}_nodeos" 1> /dev/null
echo "[Docker Container Creation Complete][${COMPOSE_PROJECT_NAME}]"
