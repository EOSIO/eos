#!/bin/bash

set -euo pipefail

buildkite-agent artifact download '*.deb' --step ':ubuntu: Ubuntu 18.04 - Package Builder' .
echo ":done: download successful"

SANITIZED_BRANCH=$(echo "$BUILDKITE_BRANCH" | sed 's.^/..' | sed 's/[:/]/_/g')
SANITIZED_TAG=$(echo "$BUILDKITE_TAG" | sed 's.^/..' | tr '/' '_')
echo "$SANITIZED_BRANCH"
echo "$SANITIZED_TAG"

# do docker build
echo ":docker::build: Building image..."
DOCKERHUB_REGISTRY="docker.io/eosio/eos"

DOCKER_BUILD_GEN="docker build -t eos_image  -f ./docker/dockerfile ."
echo "$ $DOCKER_BUILD_GEN"
eval $DOCKER_BUILD_GEN

#tag and push on each destination AWS & DOCKERHUB

EOSIO_REGS={"$EOSIO_REGISTRY" "$MIRROR_REGISTRY" "$DOCKERHUB_REGISTRY"}
for REG in ${EOSIO_REGS[@]}; do
    DOCKER_TAG_COMMIT="docker tag eos_image REG:$BUILDKITE_COMMIT"
    DOCKER_TAG_BRANCH="docker tag eos_image REG:$SANITIZED_BRANCH"
    echo "$ Tagging Images: \n$DOCKER_TAG_COMMIT \n$DOCKER_TAG_BRANCH"
    eval $DOCKER_TAG_COMMIT $DOCKER_TAG_BRANCH
    DOCKER_PUSH_COMMIT="docker push REG:$BUILDKITE_COMMIT"
    DOCKER_PUSH_BRANCH="docker push REG:$SANITIZED_BRANCH"
    echo "$ Pushing Images: \n$DOCKER_PUSH_COMMIT \n$DOCKER_PUSH_BRANCH"
    eval $DOCKER_PUSH_COMMIT $DOCKER_PUSH_BRANCH
    CLEAN_IMAGE_COMMIT="docker rmi REG:$BUILDKITE_COMMIT"
    CLEAN_IMAGE_BRANCH="docker rmi REG:$SANITIZED_BRANCH"
    echo "Cleaning Up: \n$CLEAN_IMAGE_COMMIT \n$CLEAN_IMAGE_BRANCH$"
    eval $CLEAN_IMAGE_COMMIT $CLEAN_IMAGE_BRANCH
    if [[ ! -z "$SANITIZED_TAG" ]]; then
        DOCKER_TAG="docker tag eos_image REG:$BUILDKITE_TAG"
        DOCKER_REM="docker rmi REG:$BUILDKITE_TAG"
        echo "$ \n Tagging Image: \n$DOCKER_TAG \n Cleaning Up: \n$DOCKER_REM"
        eval $DOCKER_TAG $DOCKER_REM
    fi
done