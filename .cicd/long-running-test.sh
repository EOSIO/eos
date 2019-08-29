#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
TEST="ctest -L long_running_tests --output-on-failure -T Test"
if [[ $(uname) == 'Darwin' ]]; then
    # You can't use chained commands in execute
    cd $BUILD_DIR
    bash -c "$TEST"
else # Linux
    ARGS=${ARGS:-"--rm --init -v $(pwd):$MOUNTED_DIR"}
    . $HELPERS_DIR/docker-hash.sh
    PRE_COMMANDS="cd $MOUNTED_DIR/build"
    [[ $IMAGE_TAG == 'centos-7.6' ]] && PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable && export PATH=/usr/lib64/ccache:\\\$PATH"
    COMMANDS="$PRE_COMMANDS && $TEST"
    echo "$ docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\""
    eval docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\"
fi