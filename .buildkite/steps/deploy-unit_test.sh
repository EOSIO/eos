#!/bin/bash

set -euo pipefail

docker stop mongo || true
docker rm mongo || true
docker volume rm cyberway-mongodb-data || true
docker volume create --name=cyberway-mongodb-data

cd Docker

IMAGETAG=${BUILDKITE_BRANCH:-master}

docker-compose up -d mongo

# Run unit-tests
sleep 10s
docker run --rm --network docker_cyberway-net -ti cyberway/cyberway:$IMAGETAG  /bin/bash -c 'export MONGO_URL=mongodb://mongo:27017; /opt/cyberway/bin/unit_test -l message -r detailed -t "!api_tests/*" -t "!abi_tests/*" -t "!bootseq_tests/*" -t "!eosio_system_tests/*" -t "!forked_tests/*" -t "!identity_tests/*" -t "!misc_tests/*" -t "!multi_index_tests/*" -t "!payloadless_tests/*" -t "!producer_schedule_tests/*" -t "!ram_tests/*" -t "!providebw_tests/*" -t "!tic_tac_toe_tests/*" -t "!wasm_tests/*"'
result=$?

docker-compose down

exit $result
