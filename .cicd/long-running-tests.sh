#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
TEST="ctest -L long_running_tests --output-on-failure -T Test"
if [[ $(uname) == 'Darwin' ]]; then # macOS
    cd $BUILD_DIR
    bash -c "$TEST"
else # linux
    ARGS=${ARGS:-"--rm --init -v $(pwd):$MOUNTED_DIR"}
    . $HELPERS_DIR/docker-hash.sh
    PRE_COMMANDS="cd $MOUNTED_DIR/build"
    [[ $IMAGE_TAG == 'centos-7.6' ]] && PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/devtoolset-7/enable && source /opt/rh/python33/enable && export PATH=/usr/lib64/ccache:\\\$PATH"
    COMMANDS="$PRE_COMMANDS && $TEST"
    # load buildkite environment variables for use in docker run
    if [[ -f $BUILDKITE_ENV_FILE ]]; then
        evars=""
        while read -r var; do
            evars="$evars --env ${var%%=*}"
        done < "$BUILDKITE_ENV_FILE"
    fi
    eval docker run $ARGS $evars $FULL_TAG bash -c \"$COMMANDS\"
fi