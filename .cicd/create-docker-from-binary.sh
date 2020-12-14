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
docker build -t "eosio:$BUILDKITE_COMMIT" -f "eosio/eos/docker/Dockerfile" "./eos/docker"
echo ":done: Done"

# do docker push

echo "Pushing Image..."
docker push "eos_18.04_image:$BUILDKITE_COMMIT"
echo ":done: Done"

echo "Cleaning up EOS_18.04_image:$BUILDKITE_COMMIT"
docker rmi "eos_18.04_image:$BUILDKITE_COMMIT" --force
echo ":done: clean"
