#!/usr/bin/env bash
set -eo pipefail
. ./.helpers

if [[ $BUILDKITE_BRANCH =~ ^release/[0-9].[0-9]+.x$ || $BUILDKITE_BRANCH =~ ^master$ || $BUILDKITE_BRANCH =~ ^develop$ || $FORCE_BINARIES_BUILD == true ]]; then
    cd ..
    export BRANCH=$(echo $BUILDKITE_BRANCH | sed 's/\//\-/')
    export EOS_BINARIES_TAG="eosio/producer:eos-binaries-$BRANCH"
    docker login -u $DOCKERHUB_USERNAME -p $DOCKERHUB_PASSWORD
    docker run --name eos-binaries-$BUILDKITE_COMMIT -v $(pwd):/workdir -e JOBS -e ENABLE_INSTALL=true -e ENABLE_PARALLEL_TESTS=false -e ENABLE_SERIAL_TESTS=false -e TRAVIS=true $FULL_TAG
    docker commit eos-binaries-$BUILDKITE_COMMIT $EOS_BINARIES_TAG
    docker commit eos-binaries-$BUILDKITE_COMMIT $EOS_BINARIES_TAG-$BUILDKITE_COMMIT
    docker push $EOS_BINARIES_TAG
    docker push $EOS_BINARIES_TAG-$BUILDKITE_COMMIT
    docker stop eos-binaries-$BUILDKITE_COMMIT && docker rm eos-binaries-$BUILDKITE_COMMIT 
else
    echo "This pipeline will only generate images against master, develop, and release branches. Exiting..."
    exit 0
fi
