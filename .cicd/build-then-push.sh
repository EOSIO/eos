#!/bin/bash
set -e
. ./.cicd/.helpers
# variables
export COMMIT="$(git log | head -n 1 | awk '{print $2}')"
export SANITIZED_BRANCH="$(git branch --show-current | tr '/' '_')" # '/' messes with URLs
[[ "$BUILDKITE" != 'true' ]] && export SANITIZED_TAG="$(echo $BUILDKITE_TAG | tr '/' '_')"
# build
execute docker build -f ./.cicd/docker/ubuntu-18.04-build.dockerfile -t eosio/ci-contracts-builder:base-ubuntu-18.04-latest .
# tag
execute docker tag eosio/ci-contracts-builder:base-ubuntu-18.04-latest eosio/ci-contracts-builder:base-ubuntu-18.04-$COMMIT
execute docker tag eosio/ci-contracts-builder:base-ubuntu-18.04-latest eosio/ci-contracts-builder:base-ubuntu-18.04-$SANITIZED_BRANCH
[[ "$BUILDKITE" != 'true' && ! -z "$SANITIZED_TAG" ]] && execute docker tag eosio/ci-contracts-builder:base-ubuntu-18.04-latest eosio/ci-contracts-builder:base-ubuntu-18.04-$SANITIZED_TAG
# push
if [[ "$TRAVIS" != 'true' ]]; then
    [[ "$BUILDKITE" != 'true' ]] && docker login || echo "$DOCKER_PASSWORD" | docker login -u "$DOCKER_USERNAME" --password-stdin
    execute docker push eosio/ci-contracts-builder:base-ubuntu-18.04-latest
    execute docker push eosio/ci-contracts-builder:base-ubuntu-18.04-$COMMIT
    execute docker push eosio/ci-contracts-builder:base-ubuntu-18.04-$SANITIZED_BRANCH
    [[ "$BUILDKITE" != 'true' && ! -z "$SANITIZED_TAG" ]] && execute docker push eosio/ci-contracts-builder:base-ubuntu-18.04-$SANITIZED_TAG
    # attach build.tar.gz artifact
    [[ "$BUILDKITE" == 'true' ]] && buildkite-agent artifact upload build.tar.gz
fi