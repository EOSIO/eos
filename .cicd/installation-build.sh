#!/usr/bin/env bash
set -eo pipefail
. ./.helpers

# TODO: Add if statement to gate this only running on release/ , develop , and master $BUILDKITE_BRANCH.
cd ..
determine_eos_version
execute docker run --name eos-binaries -v $(pwd):/workdir -e JOBS -e ENABLE_INSTALL=true -e ENABLE_PARALLEL_TESTS=false -e ENABLE_SERIAL_TESTS=false -e TRAVIS=true $FULL_TAG

export BINARIES_TAG_HASH="eosio/eos-binaries-$BUILDKITE_BRANCH-$EOSIO_VERSION_MAJOR.$EOSIO_VERSION_MINOR.$EOSIO_VERSION_PATCH:$BUILDKITE_COMMIT"
export BINARIES_TAG_LATEST="eosio/eos-binaries-$BUILDKITE_BRANCH-$EOSIO_VERSION_MAJOR.$EOSIO_VERSION_MINOR.$EOSIO_VERSION_PATCH:latest"
docker login -u $DOCKERHUB_USERNAME -p $DOCKERHUB_PASSWORD
docker tag eos-binaries $BINARIES_TAG_HASH
docker tag eos-binaries $BINARIES_TAG_LATEST
docker push $BINARIES_TAG_HASH
docker push $BINARIES_TAG_LATEST

