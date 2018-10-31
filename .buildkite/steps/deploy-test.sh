#/bin/bash

set -euo pipefail

docker stop keosd || true
docker stop nodeosd || true
docker stop mongo || true
docker rm keosd || true
docker rm nodeosd || true
docker rm mongo || true
docker volume rm cyberway-keosd-data || true
docker volume rm cyberway-mongodb-data || true
docker volume rm cyberway-nodeos-data || true
docker volume create --name=cyberway-keosd-data
docker volume create --name=cyberway-mongodb-data
docker volume create --name=cyberway-nodeos-data

cd Docker

IMAGETAG=${BUILDKITE_BRANCH:-master}

if [[ "$IMAGETAG" != "master" ]]; then
    echo ":llama: Change docker-compose.yml"
    sed -i "s/cyberway\/cyberway:master/cyberway\/cyberway:${IMAGETAG}/g" docker-compose.yml
    cat docker-compose.yml
fi

docker-compose up -d
sleep 10s

curl -s http://127.0.0.1:8888/v1/chain/get_info | jq '.'

docker logs nodeosd

echo -e "\nGet cleos version:"
./cleos.sh version client

docker-compose down
