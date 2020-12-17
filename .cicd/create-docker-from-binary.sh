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

echo ${MIRROR_REGISTRY}
echo ${BUILDKITE_COMMIT}

echo "Building...."
docker build -t "${MIRROR_REGISTRY}/eosio_18.04-bin:${BUILDKITE_COMMIT}" -f "./docker/Dockerfile" .
echo ":done: Done"

docker images

wait

echo "Tag Images to be pushed...."
docker tag eosio_18.04-bin:${BUILDKITE_COMMIT} ${MIRROR_REGISTRY}/eosio_18.04-bin:${BUILDKITE_COMMIT}
echo "done.. image tagged"

# do docker push

#echo "Pushing Image DockerHub..."
#docker push "eosio_18.04:$BUILDKITE_COMMIT"   
#echo ":done: Done"

DOCKER_PUSH_COMMAND="docker push '$REGISTRY:$PREFIX-$SANITIZED_TAG'"
echo "$ $DOCKER_PUSH_COMMAND"

#echo "Pushing Image to ECR..."
#echo "docker push eosio_18.04-bin:${BUILDKITE_COMMIT} to ${MIRROR_REGISTRY}"
#docker push ${MIRROR_REGISTRY}:eosio_18.04-bin
#echo ":done: Done"

#echo "Cleaning up EOS_18.04:$BUILDKITE_COMMIT"
#docker rmi "eosio_18.04:$BUILDKITE_COMMIT" --force

#echo ":done: clean"
