#!/usr/bin/env bash
set -eo pipefail
. ./.helpers

# TODO: Add if statement to gate this only running on release/ , develop , and master $BUILDKITE_BRANCH.
cd ..
determine_eos_version
execute docker run --name eos-binaries-$BUILDKITE_COMMIT -v $(pwd):/workdir -e JOBS -e ENABLE_INSTALL=true -e ENABLE_PARALLEL_TESTS=false -e ENABLE_SERIAL_TESTS=false -e TRAVIS=true $FULL_TAG

export EOS_BINARIES_TAG="eosio/producer:eos-binaries-$BUILDKITE_BRANCH-$EOSIO_VERSION_MAJOR.$EOSIO_VERSION_MINOR.$EOSIO_VERSION_PATCH"
docker login -u $DOCKERHUB_USERNAME -p $DOCKERHUB_PASSWORD
docker tag eos-binaries-$BUILDKITE_COMMIT $EOS_BINARIES_TAG
docker push $EOS_BINARIES_TAG
docker push $EOS_BINARIES_TAG-$BUILDKITE_COMMIT

docker stop eos-binaries-$BUILDKITE_COMMIT && docker rm eos-binaries-$BUILDKITE_COMMIT 
