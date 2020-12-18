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

IMAGE="$MIRROR_REGISTRY:eosio-ubuntu-18.04-$BUILDKITE_COMMIT-bin"

echo ":docker: Building Image...."
#docker build -t "${MIRROR_REGISTRY}/eosio_18.04-bin:${BUILDKITE_COMMIT}" -f "./docker/Dockerfile" .
docker build -t $IMAGE -f ./docker/Dockerfile .
echo "Build Done!!"

#echo "Tag Images to be pushed...."
#docker tag eosio_18.04-bin:${BUILDKITE_COMMIT} ${MIRROR_REGISTRY}/eosio_18.04-bin:${BUILDKITE_COMMIT}
#echo "done.. image tagged"

# do docker push
#echo "Pushing Image DockerHub..."
#docker push "eosio_18.04:$BUILDKITE_COMMIT"   
#echo ":done: Done"

#DOCKER_PUSH_COMMAND="docker push '$REGISTRY:$PREFIX-$SANITIZED_TAG'"
#echo "$ $DOCKER_PUSH_COMMAND"

#echo "Pushing Image to ECR..."
#echo "docker push eosio_18.04-bin:${BUILDKITE_COMMIT} to ${MIRROR_REGISTRY}"
#docker push ${MIRROR_REGISTRY}:eosio_18.04-bin

docker push $IMAGE
echo ":done: Done"

echo "Cleaning up ${IMAGE}"
#docker rmi "eosio_18.04:$BUILDKITE_COMMIT" --force
docker rmi $IMAGE
echo ":clean: clean"
