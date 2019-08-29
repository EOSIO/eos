#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
if [[ $(uname) == 'Darwin' ]]; then # macOS
    ./scripts/serial-test.sh
else # Linux
    . $HELPERS_DIR/docker-hash.sh
    echo "$ docker run --rm --init -v $(pwd):$MOUNTED_DIR $(buildkite-intrinsics) $FULL_TAG bash -c \"$MOUNTED_DIR/scripts/serial-test.sh\""
    eval docker run --rm --init -v $(pwd):$MOUNTED_DIR $(buildkite-intrinsics) $FULL_TAG bash -c "$MOUNTED_DIR/scripts/serial-test.sh"
fi