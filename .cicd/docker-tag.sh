#!/bin/bash
set -eo pipefail
echo '--- :evergreen_tree: Configuring Environment'
. ./.cicd/helpers/general.sh
PREFIX='base-ubuntu-18.04'
SANITIZED_BRANCH=$(echo "$BUILDKITE_BRANCH" | sed 's.^/..' | sed 's/[:/]/_/g')
SANITIZED_TAG=$(echo "$BUILDKITE_TAG" | sed 's.^/..' | tr '/' '_')
echo '$ echo ${#CONTRACT_REGISTRIES[*]} # array length'
echo ${#CONTRACT_REGISTRIES[*]}
echo '$ echo ${CONTRACT_REGISTRIES[*]} # array'
echo ${CONTRACT_REGISTRIES[*]}
# pull
echo '+++ :arrow_down: Pulling Container(s)'
for REGISTRY in ${CONTRACT_REGISTRIES[*]}; do
    if [[ ! -z "$REGISTRY" ]]; then
        echo "Pulling from '$REGISTRY'."
        IMAGE="$REGISTRY:$PREFIX-$BUILDKITE_COMMIT-$PLATFORM_TYPE"
        DOCKER_PULL_COMMAND="docker pull '$IMAGE'"
        echo "$ $DOCKER_PULL_COMMAND"
        eval $DOCKER_PULL_COMMAND
    fi
done
# tag
echo '+++ :label: Tagging Container(s)'
for REGISTRY in ${CONTRACT_REGISTRIES[*]}; do
    if [[ ! -z "$REGISTRY" ]]; then
        echo "Tagging for registry $REGISTRY."
        IMAGE="$REGISTRY:$PREFIX-$BUILDKITE_COMMIT-$PLATFORM_TYPE"
        DOCKER_TAG_COMMAND="docker tag '$IMAGE' '$REGISTRY:$PREFIX-$SANITIZED_BRANCH'"
        echo "$ $DOCKER_TAG_COMMAND"
        eval $DOCKER_TAG_COMMAND
        if [[ ! -z "$BUILDKITE_TAG" && "$SANITIZED_BRANCH" != "$SANITIZED_TAG" ]]; then
            DOCKER_TAG_COMMAND="docker tag '$IMAGE' '$REGISTRY:$PREFIX-$SANITIZED_TAG'"
            echo "$ $DOCKER_TAG_COMMAND"
            eval $DOCKER_TAG_COMMAND
        fi
    fi
done
# push
echo '+++ :arrow_up: Pushing Container(s)'
for REGISTRY in ${CONTRACT_REGISTRIES[*]}; do
    if [[ ! -z "$REGISTRY" ]]; then
        echo "Pushing to '$REGISTRY'."
        DOCKER_PUSH_COMMAND="docker push '$REGISTRY:$PREFIX-$SANITIZED_BRANCH'"
        echo "$ $DOCKER_PUSH_COMMAND"
        eval $DOCKER_PUSH_COMMAND
        if [[ ! -z "$BUILDKITE_TAG" && "$SANITIZED_BRANCH" != "$SANITIZED_TAG" ]]; then
            DOCKER_PUSH_COMMAND="docker push '$REGISTRY:$PREFIX-$SANITIZED_TAG'"
            echo "$ $DOCKER_PUSH_COMMAND"
            eval $DOCKER_PUSH_COMMAND
        fi
    fi
done
# cleanup
echo '--- :put_litter_in_its_place: Cleaning Up'
for REGISTRY in ${CONTRACT_REGISTRIES[*]}; do
    if [[ ! -z "$REGISTRY" ]]; then
        echo "Cleaning up from $REGISTRY."
        DOCKER_RMI_COMMAND="docker rmi '$REGISTRY:$PREFIX-$SANITIZED_BRANCH'"
        echo "$ $DOCKER_RMI_COMMAND"
        eval $DOCKER_RMI_COMMAND
        if [[ ! -z "$BUILDKITE_TAG" && "$SANITIZED_BRANCH" != "$SANITIZED_TAG" ]]; then
            DOCKER_RMI_COMMAND="docker rmi '$REGISTRY:$PREFIX-$SANITIZED_TAG'"
            echo "$ $DOCKER_RMI_COMMAND"
            eval $DOCKER_RMI_COMMAND
        fi
        DOCKER_RMI_COMMAND="docker rmi '$REGISTRY:$PREFIX-$BUILDKITE_COMMIT-$PLATFORM_TYPE'"
        echo "$ $DOCKER_RMI_COMMAND"
        eval $DOCKER_RMI_COMMAND
    fi
done
