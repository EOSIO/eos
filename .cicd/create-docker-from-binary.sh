#!/bin/bash
echo '--- :evergreen_tree: Configuring Environment'
set -euo pipefail
buildkite-agent artifact download '*.deb' --step ':ubuntu: Ubuntu 18.04 - Package Builder' .
echo ":done: download successful"
SANITIZED_BRANCH="$(sanitize "$BUILDKITE_BRANCH")"
echo "Branch '$BUILDKITE_BRANCH' sanitized as '$SANITIZED_BRANCH'."
SANITIZED_TAG="$(sanitize "$BUILDKITE_TAG")"
[[ -z "$SANITIZED_TAG" ]] || echo "Branch '$BUILDKITE_TAG' sanitized as '$SANITIZED_TAG'."
# docker build
echo "+++ :docker: Build Docker Container"
DOCKERHUB_REGISTRY="docker.io/eosio/eosio"
BUILD_TAG=${BUILDKITE_BUILD_NUMBER:-latest}
DOCKER_BUILD_GEN="docker build -t eosio_image:$BUILD_TAG -f ./docker/dockerfile ."
echo "$ $DOCKER_BUILD_GEN"
eval $DOCKER_BUILD_GEN
# docker tag
echo '--- :label: Tag Container'
EOSIO_REGS=("$EOSIO_REGISTRY" "$DOCKERHUB_REGISTRY")
for REG in ${EOSIO_REGS[@]}; do
    DOCKER_TAG_COMMIT="docker tag eosio_image:$BUILD_TAG $REG:$BUILDKITE_COMMIT"
    DOCKER_TAG_BRANCH="docker tag eosio_image:$BUILD_TAG $REG:$SANITIZED_BRANCH"
    echo -e "$ Tagging Images: \n$DOCKER_TAG_COMMIT \n$DOCKER_TAG_BRANCH"
    eval $DOCKER_TAG_COMMIT 
    eval $DOCKER_TAG_BRANCH
    if [[ ! -z "$SANITIZED_TAG" ]]; then
        DOCKER_TAG="docker tag eosio_image:$BUILD_TAG $REG:$SANITIZED_TAG"
        echo -e "$ \n Tagging Image: \n$DOCKER_TAG"
        eval $DOCKER_TAG
    fi
done
# docker push
echo '--- :arrow_up: Push Container'
for REG in ${EOSIO_REGS[@]}; do
    DOCKER_PUSH_COMMIT="docker push $REG:$BUILDKITE_COMMIT"
    DOCKER_PUSH_BRANCH="docker push $REG:$SANITIZED_BRANCH"
    echo -e "$ Pushing Images: \n$DOCKER_PUSH_COMMIT \n$DOCKER_PUSH_BRANCH"
    eval $DOCKER_PUSH_COMMIT 
    eval $DOCKER_PUSH_BRANCH
    if [[ ! -z "$SANITIZED_TAG" ]]; then
        DOCKER_PUSH_TAG="docker push $REG:$SANITIZED_TAG"
        eval $DOCKER_PUSH_TAG
    fi
done
# docker rmi
echo '--- :put_litter_in_its_place: Cleanup'
for REG in ${EOSIO_REGS[@]}; do
    CLEAN_IMAGE_COMMIT="docker rmi $REG:$BUILDKITE_COMMIT || :"
    CLEAN_IMAGE_BRANCH="docker rmi $REG:$SANITIZED_BRANCH || :"
    echo -e "Cleaning Up: \n$CLEAN_IMAGE_COMMIT \n$CLEAN_IMAGE_BRANCH$"
    eval $CLEAN_IMAGE_COMMIT 
    eval $CLEAN_IMAGE_BRANCH
    if [[ ! -z "$SANITIZED_TAG" ]]; then
        DOCKER_REM="docker rmi $REG:$SANITIZED_TAG || :"
        echo -e "$ Cleaning Up: \n$DOCKER_REM"
        eval $DOCKER_REM
    fi
done
DOCKER_GEN="docker rmi eosio_image:$BUILD_TAG || :"
echo "Clean up base image"
eval $DOCKER_GEN
