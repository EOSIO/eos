#!/usr/bin/env bash
set -eo pipefail
. ./.helpers

cd ..
execute docker run --name eos-binaries -v $(pwd):/workdir -e JOBS -e ENABLE_INSTALL=true -e ENABLE_PARALLEL_TESTS=false -e ENABLE_SERIAL_TESTS=false -e TRAVIS=true $FULL_TAG

docker login -u $DOCKERHUB_USERNAME -p $DOCKERHUB_PASSWORD
docker tag eos-binaries eosio/producer:eos-binaries-$HASHED_IMAGE_TAG
docker push eosio/producer:eos-binaries-$HASHED_IMAGE_TAG
