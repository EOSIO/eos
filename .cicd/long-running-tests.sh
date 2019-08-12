#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh

TEST="fold-execute ctest -L long_running_tests --output-on-failure -T Test"

if [[ $(uname) == 'Darwin' ]]; then

    # You can't use chained commands in execute
    fold-execute cd $BUILD_DIR
    fold-execute $TEST

else # Linux

    MOUNTED_DIR='/workdir'
    ARGS=${ARGS:-"--rm -v $(pwd):$MOUNTED_DIR"}

    . $HELPERS_DIR/docker-hash.sh

    PRE_COMMANDS=". $MOUNTED_DIR/.cicd/helpers/logging.sh && cd $MOUNTED_DIR/build"

    COMMANDS="$PRE_COMMANDS && $TEST"

    # Load BUILDKITE Environment Variables for use in docker run
    if [[ -f $BUILDKITE_ENV_FILE ]]; then
        evars=""
        while read -r var; do
            evars="$evars --env ${var%%=*}"
        done < "$BUILDKITE_ENV_FILE"
    fi

    fold-execute eval docker run $ARGS $evars $FULL_TAG bash -c \"$COMMANDS\"

fi