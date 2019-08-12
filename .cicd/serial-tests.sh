#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh

TEST="mkdir -p ./mongodb && mongod --dbpath ./mongodb --fork --logpath mongod.log && ctest -L nonparallelizable_tests --output-on-failure -T Test"

if [[ $(uname) == 'Darwin' ]]; then

    # You can't use chained commands in execute
    cd $BUILD_DIR
    $TEST

else # Linux

    MOUNTED_DIR='/workdir'
    ARGS=${ARGS:-"--rm -v $(pwd):$MOUNTED_DIR"}

    . $HELPERS_DIR/docker-hash.sh

    PRE_COMMANDS="cd $MOUNTED_DIR/build"
    [[ $IMAGE_TAG == 'centos-7.6' ]] && PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable && export PATH=/usr/lib64/ccache:\\\$PATH"

    COMMANDS="$PRE_COMMANDS && $TEST"

    # Load BUILDKITE Environment Variables for use in docker run
    if [[ -f $BUILDKITE_ENV_FILE ]]; then
        evars=""
        while read -r var; do
            evars="$evars --env ${var%%=*}"
        done < "$BUILDKITE_ENV_FILE"
    fi

    eval docker run $ARGS $evars $FULL_TAG bash -c \"$COMMANDS\"

fi