#!/bin/bash

set -e

env

curl google.com
curl archive.ubuntu.com

docker build -t eosio/test ./Docker

docker tag eosio/test gcr.io/b1ops-devel/eosio/test
docker push gcr.io/b1ops-devel/eosio/test

docker rmi eosio/test
docker rmi gcr.io/b1ops-devel/eosio/test
