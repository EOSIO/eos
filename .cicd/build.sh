#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
export DOCKERIZATION=false
[[ $ENABLE_INSTALL == true ]] && . ./.cicd/helpers/populate-template-and-hash.sh '<!-- BUILD -->' '<!-- INSTALL END' || . ./.cicd/helpers/populate-template-and-hash.sh '<!-- BUILD'
sed -i -e "s/# Commands from the documentation are inserted right below this line/cd \$EOSIO_LOCATION \&\& git pull \&\& git checkout -f $BUILDKITE_COMMIT && git submodule update --init --recursive/g" /tmp/$POPULATED_FILE_NAME
if [[ "$(uname)" == 'Darwin' ]]; then
    # You can't use chained commands in execute
    if [[ $TRAVIS == true ]]; then
        ccache -s
        brew reinstall openssl@1.1 # Fixes issue where builds in Travis cannot find libcrypto.
        sed -i -e 's/^cmake /cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache /g' /tmp/$POPULATED_FILE_NAME
    fi
    . ./tmp/$POPULATED_FILE_NAME # This file is populated from the platform's build documentation code block
else # Linux
    ARGS=${ARGS:-"--rm --init -v /tmp/$POPULATED_FILE_NAME:/build-script"}
    # PRE_COMMANDS: Executed pre-cmake
    [[ ! $IMAGE_TAG =~ 'unpinned' ]] && CMAKE_EXTRAS="-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    if [[ $IMAGE_TAG == 'amazon_linux-2-pinned' ]]; then
        PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
    elif [[ "$IMAGE_TAG" == 'centos-7.7-pinned' ]]; then
        PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
    elif [[ $IMAGE_TAG == 'ubuntu-16.04-pinned' ]]; then
        PRE_COMMANDS="export PATH=/usr/lib/ccache:\\\$PATH"
    elif [[ $IMAGE_TAG == 'ubuntu-18.04-pinned' ]]; then
        PRE_COMMANDS="export PATH=/usr/lib/ccache:\\\$PATH"
    elif [[ $IMAGE_TAG == 'amazon_linux-2-unpinned' ]]; then
        PRE_COMMANDS="export PATH=/usr/lib64/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang'"
    elif [[ "$IMAGE_TAG" == 'centos-7.7-unpinned' ]]; then
        PRE_COMMANDS="source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable && export PATH=/usr/lib64/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DLLVM_DIR='/opt/rh/llvm-toolset-7.0/root/usr/lib64/cmake/llvm'"
    elif [[ $IMAGE_TAG == 'ubuntu-18.04-unpinned' ]]; then
        PRE_COMMANDS="export PATH=/usr/lib/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++-7' -DCMAKE_C_COMPILER='clang-7' -DLLVM_DIR='/usr/lib/llvm-7/lib/cmake/llvm'"
    fi
    BUILD_COMMANDS="/build-script"
    if [[ $TRAVIS == true ]]; then
        ARGS="$ARGS -v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e JOBS -e TRAVIS -e CCACHE_DIR=/opt/.ccache"
        BUILD_COMMANDS="ccache -s && $BUILD_COMMANDS"
    fi
    . $HELPERS_DIR/populate-template-and-hash.sh -h # obtain $FULL_TAG (and don't overwrite existing file)
    echo "------------------------"
    echo "$ docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$PRE_COMMANDS && $BUILD_COMMANDS\""
    eval docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$PRE_COMMANDS \&\& $BUILD_COMMANDS\"
fi