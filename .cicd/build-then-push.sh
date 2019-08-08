#!/bin/bash
set -e
. ./.cicd/.helpers
# build
execute docker build -f ./.cicd/ubuntu-18.04-build.dockerfile -t eosio/ci-contracts-builder:base-ubuntu-18.04-$(git log | head -n 1 | awk '{print $2}') .
# push
echo "$DOCKER_PASSWORD" | docker login -u "$DOCKER_USERNAME" --password-stdin
execute docker push eosio/ci-contracts-builder:base-ubuntu-18.04-$(git log | head -n 1 | awk '{print $2}')