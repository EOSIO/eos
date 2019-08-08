#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh

if [[ $BUILDKITE_BRANCH =~ ^release/[0-9].[0-9]+.x$ || $BUILDKITE_BRANCH =~ ^master$ || $BUILDKITE_BRANCH =~ ^develop$ || $FORCE_BINARIES_BUILD == true ]]; then
    determine_eos_version
    export EOS_BINARIES_TAG="eosio/producer:eos-binaries-$BUILDKITE_BRANCH-$EOSIO_VERSION_MAJOR.$EOSIO_VERSION_MINOR.$EOSIO_VERSION_PATCH"
    export ARGS="--name eos-binaries-$BUILDKITE_COMMIT -v $(pwd):$MOUNTED_DIR"
    docker login -u $DOCKERHUB_USERNAME -p $DOCKERHUB_PASSWORD
    ./.cicd/run.sh
    docker commit eos-binaries-$BUILDKITE_COMMIT $EOS_BINARIES_TAG
    docker commit eos-binaries-$BUILDKITE_COMMIT $EOS_BINARIES_TAG-$BUILDKITE_COMMIT
    docker push $EOS_BINARIES_TAG
    docker push $EOS_BINARIES_TAG-$BUILDKITE_COMMIT
    docker stop eos-binaries-$BUILDKITE_COMMIT && docker rm eos-binaries-$BUILDKITE_COMMIT 
else
    echo "This pipeline will only generate images against master, develop, and release branches. Exiting..."
    exit 0
fi
