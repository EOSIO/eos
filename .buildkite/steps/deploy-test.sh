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

echo ":llama: Change docker-compose.yml"
sed -i "s/cyberway\/cyberway:stable/cyberway\/cyberway:${IMAGETAG}/g" docker-compose.yml
sed -i "s/--genesis-json \S\+ --genesis-data \S\+//g" docker-compose.yml
sed -i "s/\${PWD}\/config.ini/\${PWD}\/config-standalone.ini/g" docker-compose.yml
echo "----------------------------------------------"
cat docker-compose.yml
echo "----------------------------------------------"

docker-compose up -d || true
sleep 15s

NODE_STATUS=$(docker inspect --format "{{.State.Status}}" nodeosd)
NODE_EXIT=$(docker inspect --format "{{.State.ExitCode}}" nodeosd)

if [[ "$NODE_STATUS" == "exited" ]] || [[ "$NODE_EXIT" != "0" ]]; then
    docker logs nodeosd
    exit 1
fi

curl -s http://127.0.0.1:8888/v1/chain/get_info | jq '.'

docker logs nodeosd

echo -e "\nGet cleos version:"
./cleos.sh version client

docker-compose down
