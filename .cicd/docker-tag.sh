#!/bin/bash
set -eo pipefail
echo '+++ :evergreen_tree: Configuring Environment'
REPO='eosio/ci-contracts-builder'
PREFIX='base-ubuntu-18.04'
IMAGE="$REPO:$PREFIX-$BUILDKITE_COMMIT"
SANITIZED_BRANCH=$(echo "$BUILDKITE_BRANCH" | tr '/' '_')
SANITIZED_TAG=$(echo "$BUILDKITE_TAG" | tr '/' '_')
echo '+++ :arrow_down: Pulling Container'
echo "Pulling \"$IMAGE\""
docker pull "$IMAGE"
echo '+++ :label: Tagging Container'
docker tag "$IMAGE" "$REPO:$PREFIX-$SANITIZED_BRANCH"
echo "Tagged \"$REPO:$PREFIX-$SANITIZED_BRANCH\"."
[[ -z "$BUILDKITE_TAG" ]] || docker tag "$IMAGE" "$REPO:$PREFIX-$SANITIZED_TAG" && echo "Tagged \"$REPO:$PREFIX-$SANITIZED_TAG\"."
echo '+++ :arrow_up: Pushing Container'
docker push "$REPO:$PREFIX-$SANITIZED_BRANCH"
[[ -z "$BUILDKITE_TAG" ]] || docker push "$REPO:$PREFIX-$SANITIZED_TAG"
echo '+++ :put_litter_in_its_place: Cleaning Up'
docker rmi "$REPO:$PREFIX-$SANITIZED_BRANCH"
[[ -z "$BUILDKITE_TAG" || "$SANITIZED_BRANCH" == "$SANITIZED_TAG" ]] || docker rmi "$REPO:$PREFIX-$SANITIZED_TAG"
docker rmi "$IMAGE"