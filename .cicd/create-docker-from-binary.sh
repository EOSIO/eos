#!/bin/bash
echo '--- :evergreen_tree: Configuring Environment'
set -euo pipefail
. ./.cicd/helpers/general.sh
buildkite-agent artifact download '*.deb' --step ':ubuntu: Ubuntu 18.04 - Package Builder' .
SANITIZED_BRANCH="$(sanitize "$BUILDKITE_BRANCH")"
echo "Branch '$BUILDKITE_BRANCH' sanitized as '$SANITIZED_BRANCH'."
SANITIZED_TAG="$(sanitize "$BUILDKITE_TAG")"
[[ -z "$SANITIZED_TAG" ]] || echo "Branch '$BUILDKITE_TAG' sanitized as '$SANITIZED_TAG'."
# docker build
echo "+++ :docker: Build Docker Container"
DOCKERHUB_REGISTRY='docker.io/eosio/eosio'
IMAGE="${DOCKERHUB_REGISTRY}:${BUILDKITE_COMMIT:-latest}"
DOCKER_BUILD="docker build -t '$IMAGE' -f ./docker/dockerfile ."
echo "$ $DOCKER_BUILD"
eval $DOCKER_BUILD
# docker tag
echo '--- :label: Tag Container'
if [[ "$BUILDKITE_PIPELINE_SLUG" =~ "-sec" ]] ; then
    REGISTRIES=("$EOSIO_REGISTRY")
else
    REGISTRIES=("$EOSIO_REGISTRY" "$DOCKERHUB_REGISTRY")
fi
for REG in ${REGISTRIES[@]}; do
    DOCKER_TAG_BRANCH="docker tag '$IMAGE' '$REG:$SANITIZED_BRANCH'"
    echo "$ $DOCKER_TAG_BRANCH"
    eval $DOCKER_TAG_BRANCH
    DOCKER_TAG_COMMIT="docker tag '$IMAGE' '$REG:$BUILDKITE_COMMIT'"
    echo "$ $DOCKER_TAG_COMMIT"
    eval $DOCKER_TAG_COMMIT
    if [[ ! -z "$SANITIZED_TAG" && "$SANITIZED_BRANCH" != "$SANITIZED_TAG" ]]; then
        DOCKER_TAG="docker tag '$IMAGE' '$REG:$SANITIZED_TAG'"
        echo "$ $DOCKER_TAG"
        eval $DOCKER_TAG
    fi
done
# docker push
echo '--- :arrow_up: Push Container'
for REG in ${REGISTRIES[@]}; do
    DOCKER_PUSH_BRANCH="docker push '$REG:$SANITIZED_BRANCH'"
    echo "$ $DOCKER_PUSH_BRANCH"
    eval $DOCKER_PUSH_BRANCH
    DOCKER_PUSH_COMMIT="docker push '$REG:$BUILDKITE_COMMIT'"
    echo "$ $DOCKER_PUSH_COMMIT"
    eval $DOCKER_PUSH_COMMIT
    if [[ ! -z "$SANITIZED_TAG" && "$SANITIZED_BRANCH" != "$SANITIZED_TAG" ]]; then
        DOCKER_PUSH_TAG="docker push '$REG:$SANITIZED_TAG'"
        echo "$ $DOCKER_PUSH_TAG"
        eval $DOCKER_PUSH_TAG
    fi
done
# docker rmi
echo '--- :put_litter_in_its_place: Cleanup'
for REG in ${REGISTRIES[@]}; do
    CLEAN_IMAGE_BRANCH="docker rmi '$REG:$SANITIZED_BRANCH' || :"
    echo "$ $CLEAN_IMAGE_BRANCH"
    eval $CLEAN_IMAGE_BRANCH
    CLEAN_IMAGE_COMMIT="docker rmi '$REG:$BUILDKITE_COMMIT' || :"
    echo "$ $CLEAN_IMAGE_COMMIT"
    eval $CLEAN_IMAGE_COMMIT
    if [[ ! -z "$SANITIZED_TAG" && "$SANITIZED_BRANCH" != "$SANITIZED_TAG" ]]; then
        DOCKER_RMI="docker rmi '$REG:$SANITIZED_TAG' || :"
        echo "$ $DOCKER_RMI"
        eval $DOCKER_RMI
    fi
done
DOCKER_RMI="docker rmi '$IMAGE' || :"
echo "$ $DOCKER_RMI"
eval $DOCKER_RMI
