#!/bin/bash

set -euo pipefail

echo ":docker::package: Push EOS 18.04 Image to Dockerhub"

# buildkite-agent artifact download someversion.deb .

echo ":download: Downloading artifact from Buildkite step Ubuntu 18.04"
buildkite-agent artifact download '*.deb' --step ':ubuntu: Ubuntu 18.04 - Package Builder' .
echo ":done: download successfull"

# do docker build

echo ":docker::build: Build image"

set -e

echo "Building...."
docker build -t "eosio_18.04:$BUILDKITE_COMMIT-bin" -f "./docker/Dockerfile" .
echo ":done: Done"

# do docker push

#echo "Pushing Image DockerHub..."
#docker push "eosio_18.04:$BUILDKITE_COMMIT"   
#echo ":done: Done"

$BUILDED_IMAGE=eosio_18.04:$BUILDKITE_COMMIT-bin
echo "Pushing Image to ECR..."
docker push '$MIRROR_REGISTRY:$BUILDED_IMAGE' 
echo ":done: Done"

echo "Cleaning up EOS_18.04:$BUILDKITE_COMMIT"
docker rmi "eosio_18.04:$BUILDKITE_COMMIT" --force
echo ":done: clean"
