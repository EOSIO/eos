#!/bin/bash

set -e

export http_proxy=http://proxy.service:3128
export https_proxy=http://proxy.service:3128

curl -m 3 google.com
curl -m 3 archive.ubuntu.com

docker build --network=host -t eosio/test-registry ./Docker

docker login registry.devel.b1ops.net --username $DOCKER_REGISTRY_USERNAME --password $DOCKER_REGISTRY_TOKEN

docker tag eosio/test-registry registry.devel.b1ops.net/eosio/test-registry
docker push registry.devel.b1ops.net/eosio/test-registry

docker rmi eosio/test-registry
docker rmi registry.devel.b1ops.net/eosio/test-registry
