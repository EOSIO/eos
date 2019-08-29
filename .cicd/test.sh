#!/bin/bash
set -eo pipefail
# variables
. ./.cicd/helpers/general.sh
# tests
if [[ $(uname) == 'Darwin' ]]; then # macOS
    ./"$@"
else # Linux
    . $HELPERS_DIR/docker-hash.sh
    echo "$ docker run --rm --init -v $(pwd):$MOUNTED_DIR $(buildkite-intrinsics) $FULL_TAG bash -c \"$MOUNTED_DIR/$@\""
    eval docker run --rm --init -v $(pwd):$MOUNTED_DIR $(buildkite-intrinsics) $FULL_TAG bash -c "$MOUNTED_DIR/$@"
fi