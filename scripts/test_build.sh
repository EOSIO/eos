#!/bin/bash

set -e

docker build --network=host -t eosio/test-registry ./Docker

docker tag eosio/test-registry registry.devel.b1ops.net/eosio/test-registry
docker push registry.devel.b1ops.net/eosio/test-registry

docker rmi eosio/test-registry
docker rmi registry.devel.b1ops.net/eosio/test-registry
