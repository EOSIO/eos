#!/bin/bash

set -eo pipefail

BUILDKITE_AGENT_ARTIFACT_DOWNLOAD="buildkite-agent artifact download '*.deb' --step ':ubuntu: Ubuntu 18.04 - Package Builder' --build '${EOSIO_BUILD_ID:-$BUILDKITE_BUILD_ID}' ."
echo "$ $BUILDKITE_AGENT_ARTIFACT_DOWNLOAD"
eval $BUILDKITE_AGENT_ARTIFACT_DOWNLOAD
echo ":done: download successful"

[[ -z "$BRANCH" ]] && export BRANCH="$BUILDKITE_BRANCH"
[[ -z "$COMMIT" ]] && export COMMIT="$BUILDKITE_COMMIT"
[[ -z "$TAG" ]] && export TAG="$BUILDKITE_TAG"

SANITIZED_BRANCH=$(echo "$BRANCH" | sed 's.^/..' | sed 's/[:/]/_/g')
SANITIZED_TAG=$(echo "$TAG" | sed 's.^/..' | tr '/' '_')
echo "$SANITIZED_BRANCH"
echo "$SANITIZED_TAG"

# do docker build
echo ":docker::build: Building image..."
DOCKERHUB_REGISTRY="docker.io/eosio/eosio"

BUILD_TAG=${BUILDKITE_BUILD_NUMBER:-latest}
DOCKER_BUILD_GEN="docker build -t eosio_image:$BUILD_TAG -f ./docker/dockerfile ."
echo "$ $DOCKER_BUILD_GEN"
eval $DOCKER_BUILD_GEN

#tag and push on each destination AWS & DOCKERHUB

EOSIO_REGS=("$EOSIO_REGISTRY" "$DOCKERHUB_REGISTRY")
for REG in ${EOSIO_REGS[@]}; do
    DOCKER_TAG_COMMIT="docker tag eosio_image:$BUILD_TAG $REG:$COMMIT"
    DOCKER_TAG_BRANCH="docker tag eosio_image:$BUILD_TAG $REG:$SANITIZED_BRANCH"
    echo -e "$ Tagging Images: \n$DOCKER_TAG_COMMIT \n$DOCKER_TAG_BRANCH"
    eval $DOCKER_TAG_COMMIT 
    eval $DOCKER_TAG_BRANCH
    DOCKER_PUSH_COMMIT="docker push $REG:$COMMIT"
    DOCKER_PUSH_BRANCH="docker push $REG:$SANITIZED_BRANCH"
    echo -e "$ Pushing Images: \n$DOCKER_PUSH_COMMIT \n$DOCKER_PUSH_BRANCH"
    eval $DOCKER_PUSH_COMMIT 
    eval $DOCKER_PUSH_BRANCH
    CLEAN_IMAGE_COMMIT="docker rmi $REG:$COMMIT"
    CLEAN_IMAGE_BRANCH="docker rmi $REG:$SANITIZED_BRANCH"
    echo -e "Cleaning Up: \n$CLEAN_IMAGE_COMMIT \n$CLEAN_IMAGE_BRANCH$"
    eval $CLEAN_IMAGE_COMMIT 
    eval $CLEAN_IMAGE_BRANCH
    if [[ ! -z "$SANITIZED_TAG" ]]; then
        DOCKER_TAG="docker tag eosio_image:$BUILD_TAG $REG:$SANITIZED_TAG"
        DOCKER_REM="docker rmi $REG:$SANITIZED_TAG"
        echo -e "$ \n Tagging Image: \n$DOCKER_TAG \n Cleaning Up: \n$DOCKER_REM"
        eval $DOCKER_TAG 
        eval $DOCKER_REM
    fi
done

DOCKER_GEN="docker rmi eosio_image:$BUILD_TAG"
echo "Clean up base image"
echo "$ $DOCKER_GEN"
eval $DOCKER_GEN
