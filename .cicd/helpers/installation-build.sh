#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh

export ENABLE_INSTALL=true

if [[ $BUILDKITE_BRANCH =~ ^release/[0-9].[0-9]+.x$ || $BUILDKITE_BRANCH =~ ^master$ || $BUILDKITE_BRANCH =~ ^develop$ || $FORCE_BINARIES_BUILD == true ]]; then
    determine_eos_version
    export EOS_BINARIES_TAG="eosio/producer:eos-binaries-$BUILDKITE_BRANCH-$EOSIO_VERSION_MAJOR.$EOSIO_VERSION_MINOR.$EOSIO_VERSION_PATCH"
    export ARGS="--name eos-binaries-$BUILDKITE_COMMIT -v $(pwd):$MOUNTED_DIR"
    fold-execute docker login -u $DOCKERHUB_USERNAME -p $DOCKERHUB_PASSWORD
    $CICD_DIR/build.sh
    fold-execute docker commit eos-binaries-$BUILDKITE_COMMIT $EOS_BINARIES_TAG
    fold-execute docker commit eos-binaries-$BUILDKITE_COMMIT $EOS_BINARIES_TAG-$BUILDKITE_COMMIT
    fold-execute docker push $EOS_BINARIES_TAG
    fold-execute docker push $EOS_BINARIES_TAG-$BUILDKITE_COMMIT
    fold-execute docker stop eos-binaries-$BUILDKITE_COMMIT && docker rm eos-binaries-$BUILDKITE_COMMIT 
else
    echo "This pipeline will only generate images against master, develop, and release branches. Exiting..."
    exit 0
fi
