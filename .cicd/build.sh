#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh

function cleanup() {
    if [[ "$(uname)" != 'Darwin' ]]; then
        echo "[Cleaning up docker container]"
        echo " - docker container kill $CONTAINER_NAME"
        docker container kill $CONTAINER_NAME || true # -v and --mount don't work quite well when it's docker in docker, so we need to use docker's cp command to move the build script in
    fi
}
trap cleanup 0

export DOCKERIZATION=false
[[ $ENABLE_INSTALL == true ]] && . ./.cicd/helpers/populate-template-and-hash.sh '<!-- DAC BUILD' '<!-- DAC INSTALL' || . ./.cicd/helpers/populate-template-and-hash.sh '<!-- DAC BUILD'
if [[ "$(uname)" == 'Darwin' ]]; then
    # You can't use chained commands in execute
    if [[ $TRAVIS == true ]]; then
        ccache -s
        brew reinstall openssl@1.1 # Fixes issue where builds in Travis cannot find libcrypto.
        sed -i -e 's/^cmake /cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache /g' /tmp/$POPULATED_FILE_NAME
        sed -i -e 's/ -DCMAKE_TOOLCHAIN_FILE=$EOSIO_LOCATION\/scripts\/pinned_toolchain.cmake//g' /tmp/$POPULATED_FILE_NAME # We can't use pinned for mac cause building clang8 would take too long
        sed -i -e 's/ -DBUILD_MONGO_DB_PLUGIN=true//g' /tmp/$POPULATED_FILE_NAME # We can't use pinned for mac cause building clang8 would take too long
        # Support ship_test
        export NVM_DIR="$HOME/.nvm"
        . "/usr/local/opt/nvm/nvm.sh"
        nvm install --lts=dubnium
    else
        source ~/.bash_profile # Make sure node is available for ship_test
    fi
    . $HELPERS_DIR/populate-template-and-hash.sh -h # obtain $FULL_TAG (and don't overwrite existing file)
    cat /tmp/$POPULATED_FILE_NAME
    . /tmp/$POPULATED_FILE_NAME # This file is populated from the platform's build documentation code block
else # Linux
    if [[ $TRAVIS == true ]]; then
        ARGS=${ARGS:-"-v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e JOBS -e TRAVIS -e CCACHE_DIR=/opt/.ccache"}
        export CONTAINER_NAME=$TRAVIS_JOB_ID
        [[ ! $IMAGE_TAG =~ 'unpinned' ]] && sed -i -e 's/^cmake /cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache /g' /tmp/$POPULATED_FILE_NAME
        if [[ $IMAGE_TAG == 'amazon_linux-2-pinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'centos-7.7-pinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'ubuntu-16.04-pinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'ubuntu-18.04-pinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'amazon_linux-2-unpinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'centos-7.7-unpinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
        elif [[ $IMAGE_TAG == 'ubuntu-18.04-unpinned' ]]; then
            PRE_COMMANDS="export PATH=/usr/lib/ccache:\\\$PATH"
        fi
        BUILD_COMMANDS="ccache -s && $PRE_COMMANDS && "
    else
        export CONTAINER_NAME=$BUILDKITE_JOB_ID
    fi
    ARGS="$ARGS --rm -t -d --name $CONTAINER_NAME -v $(pwd):$MOUNTED_DIR"
    # sed -i '1s;^;#!/bin/bash\nexport PATH=$EOSIO_INSTALL_LOCATION/bin:$PATH\n;' /tmp/$POPULATED_FILE_NAME # /build-script: line 3: cmake: command not found
    # PRE_COMMANDS: Executed pre-cmake
    BUILD_COMMANDS="$BUILD_COMMANDS/$POPULATED_FILE_NAME"
    . $HELPERS_DIR/populate-template-and-hash.sh -h # obtain $FULL_TAG (and don't overwrite existing file)
    echo "$ docker run $ARGS $(buildkite-intrinsics) $FULL_TAG"
    eval docker run $ARGS $(buildkite-intrinsics) $FULL_TAG
    echo "$ docker cp /tmp/$POPULATED_FILE_NAME $CONTAINER_NAME:/$POPULATED_FILE_NAME"
    docker cp /tmp/$POPULATED_FILE_NAME $CONTAINER_NAME:/$POPULATED_FILE_NAME
    cat /tmp/$POPULATED_FILE_NAME
    echo "$ docker exec $CONTAINER_NAME bash -c \"$BUILD_COMMANDS\""
    eval docker exec $CONTAINER_NAME bash -c \"$BUILD_COMMANDS\"
fi