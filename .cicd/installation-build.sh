#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
export ENABLE_INSTALL=true
export BRANCH=$(echo $BUILDKITE_BRANCH | sed 's.^/..' | tr '/' '_')
export CONTRACTS_BUILDER_TAG="eosio/ci-contracts-builder:base-ubuntu-18.04"
export ARGS="--name ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER --init -v $(pwd):$MOUNTED_DIR"
$CICD_DIR/build.sh
for REGISTRY in "${CONTRACT_REGISTRIES[@]}"; do
  if [[ ! -z $REGISTRY ]]; then
    docker commit ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER $REGISTRY-base-ubuntu-18.04-$BUILDKITE_COMMIT
    docker commit ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER $REGISTRY-base-ubuntu-18.04-$BUILDKITE_COMMIT-$PLATFORM_TYPE
    docker commit ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER $REGISTRY-base-ubuntu-18.04-$BRANCH-$BUILDKITE_COMMIT
    docker push $REGISTRY-base-ubuntu-18.04-$BUILDKITE_COMMIT
    docker push $REGISTRY-base-ubuntu-18.04-$BUILDKITE_COMMIT-$PLATFORM_TYPE
    docker push $REGISTRY-base-ubuntu-18.04-$BRANCH-$BUILDKITE_COMMIT
    docker stop ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER
    docker rm ci-contracts-builder-$BUILDKITE_PIPELINE_SLUG-$BUILDKITE_BUILD_NUMBER    
  fi
done