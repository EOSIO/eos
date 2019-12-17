#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
export ENABLE_INSTALL=true
if [[ $BUILDKITE_BRANCH =~ ^release/[0-9]+.[0-9]+.x$ || $BUILDKITE_BRANCH =~ ^master$ || $BUILDKITE_BRANCH =~ ^develop$ || ! -z $BUILDKITE_TAG || $FORCE_BINARIES_BUILD == true ]]; then
    export BRANCH=$(echo $BUILDKITE_BRANCH | sed 's/\//\_/')
    export CONTRACTS_BUILDER_TAG="eosio/ci-contracts-builder:base-ubuntu-18.04"
    export ARGS="--name ci-contracts-builder-$BUILDKITE_COMMIT --init -v $(pwd):$MOUNTED_DIR"
    $CICD_DIR/build.sh
    docker commit ci-contracts-builder-$BUILDKITE_COMMIT $CONTRACTS_BUILDER_TAG-$BUILDKITE_COMMIT
    docker commit ci-contracts-builder-$BUILDKITE_COMMIT $CONTRACTS_BUILDER_TAG-$BRANCH
    docker commit ci-contracts-builder-$BUILDKITE_COMMIT $CONTRACTS_BUILDER_TAG-$BRANCH-$BUILDKITE_COMMIT
    docker push $CONTRACTS_BUILDER_TAG-$BUILDKITE_COMMIT
    docker push $CONTRACTS_BUILDER_TAG-$BRANCH
    docker push $CONTRACTS_BUILDER_TAG-$BRANCH-$BUILDKITE_COMMIT
    docker stop ci-contracts-builder-$BUILDKITE_COMMIT && docker rm ci-contracts-builder-$BUILDKITE_COMMIT
else
    echo "Contract builder images are only created for tags or master, develop, and release branches. Start a new build with FORCE_BINARIES_BUILD=true to create a Contract Builder image for other branches/commits. Exiting..."
    exit 0
fi