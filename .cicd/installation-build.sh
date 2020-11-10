#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
export ENABLE_INSTALL=true
export BRANCH=$(echo $BUILDKITE_BRANCH | sed 's.^/..' | sed 's/[:/]/_/g')
export CONTRACTS_BUILDER_TAG="eosio/ci-contracts-builder:base-ubuntu-18.04"
export ARGS="--name ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER --init -v $(pwd):$MOUNTED_DIR"
"$CICD_DIR/build.sh"
for REGISTRY in "${CONTRACT_REGISTRIES[@]}"; do
    if [[ ! -z $REGISTRY ]]; then
        COMMITS=("$REGISTRY:base-ubuntu-18.04-$BUILDKITE_COMMIT" "$REGISTRY:base-ubuntu-18.04-$BUILDKITE_COMMIT-$PLATFORM_TYPE" "$REGISTRY:base-ubuntu-18.04-$BRANCH-$BUILDKITE_COMMIT")
        for COMMIT in "${COMMITS[@]}"; do
            COMMIT_COMMAND="docker commit 'ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER' '$COMMIT'"
            echo "$ $COMMIT_COMMAND"
            eval $COMMIT_COMMAND
            PUSH_COMMAND="docker push '$COMMIT'"
            echo "$ $PUSH_COMMAND"
            eval $PUSH_COMMAND
        done
    fi
done
DOCKER_STOP_COMMAND="docker stop 'ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER'"
echo "$ $DOCKER_STOP_COMMAND"
eval $DOCKER_STOP_COMMAND
DOCKER_RM_COMMAND="docker rm 'ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER'"
echo "$ $DOCKER_RM_COMMAND"
eval $DOCKER_RM_COMMAND
