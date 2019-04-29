#!/bin/bash

set -e

export http_proxy=http://proxy.service:3128
export https_proxy=http://proxy.service:3128

curl -m 3 google.com
curl -m 3 archive.ubuntu.com

docker build --network=host -t eosio/test ./Docker

docker tag eosio/test gcr.io/b1ops-devel/eosio/test
docker push gcr.io/b1ops-devel/eosio/test

docker rmi eosio/test
docker rmi gcr.io/b1ops-devel/eosio/test
