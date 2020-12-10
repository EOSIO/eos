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

echo "Building $IMAGE:$SOURCE_COMMIT"
docker build -t "$IMAGE:$SOURCE_COMMIT" -f "./docker/Dockerfile" "./docker"
echo ":done: Done"

# do docker push

echo "Pushing $IMAGE:$SOURCE_COMMIT"
docker push "$IMAGE:$SOURCE_COMMIT"
echo ":done: Done"

echo "Cleaning up $IMAGE:$SOURCE_COMMIT"
docker rmi "$IMAGE:$SOURCE_COMMIT" --force
echo ":done: clean"

buildkite-agent annotate ":docker: $VERSION_NAME - Built and pushed image $IMAGE:$SOURCE_COMMIT" --style success --context "eosio-$VERSION_NAME"
echo ":done: Annotation done"