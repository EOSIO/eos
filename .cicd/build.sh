#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
mkdir -p $BUILD_DIR
CMAKE_EXTRAS="-DCMAKE_BUILD_TYPE='Release' -DENABLE_MULTIVERSION_PROTOCOL_TEST=true"
if [[ "$(uname)" == 'Darwin' && $FORCE_LINUX != true ]]; then
    # You can't use chained commands in execute
    if [[ "$GITHUB_ACTIONS" == 'true' ]]; then
        export PINNED=false
    fi
    [[ ! "$PINNED" == 'false' ]] && CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_TOOLCHAIN_FILE=$HELPERS_DIR/clang.make"
    cd $BUILD_DIR
    if [[ $TRAVIS == true ]]; then
        ccache -s
        brew link --overwrite python
        # Support ship_test
        export NVM_DIR="$HOME/.nvm"
        . "/usr/local/opt/nvm/nvm.sh"
        nvm install --lts=dubnium
    else
        source ~/.bash_profile # Make sure node is available for ship_test
    fi
    echo "cmake $CMAKE_EXTRAS .."
    cmake $CMAKE_EXTRAS ..
    echo "make -j$JOBS"
    make -j$JOBS
else # Linux
    ARGS=${ARGS:-"--rm --init -v $(pwd):$MOUNTED_DIR"}
    PRE_COMMANDS="cd $MOUNTED_DIR/build"
    # PRE_COMMANDS: Executed pre-cmake
    # CMAKE_EXTRAS: Executed within and right before the cmake path (cmake CMAKE_EXTRAS ..)
    [[ ! "$IMAGE_TAG" =~ 'unpinned' ]] && CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_TOOLCHAIN_FILE=$MOUNTED_DIR/.cicd/helpers/clang.make"
    if [[ "$IMAGE_TAG" == 'amazon_linux-2-unpinned' ]]; then
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang'"
    elif [[ "$IMAGE_TAG" == 'centos-7.7-unpinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/devtoolset-8/enable"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DLLVM_DIR='/opt/rh/llvm-toolset-7.0/root/usr/lib64/cmake/llvm'"
    elif [[ "$IMAGE_TAG" == 'ubuntu-18.04-unpinned' ]]; then
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++-7' -DCMAKE_C_COMPILER='clang-7' -DLLVM_DIR='/usr/lib/llvm-7/lib/cmake/llvm'"
    fi
    if [[ "$IMAGE_TAG" == centos* ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/rh-python36/enable"
    fi
    BUILD_COMMANDS="cmake $CMAKE_EXTRAS .. && make -j$JOBS"
    # Docker Commands
    if [[ "$BUILDKITE" == 'true' ]]; then
        # Generate Base Images
        $CICD_DIR/generate-base-images.sh
        [[ "$ENABLE_INSTALL" == 'true' ]] && COMMANDS="cp -r $MOUNTED_DIR /root/eosio && cd /root/eosio/build &&"
        COMMANDS="$COMMANDS $BUILD_COMMANDS"
        [[ "$ENABLE_INSTALL" == 'true' ]] && COMMANDS="$COMMANDS && make install"
    elif [[ "$GITHUB_ACTIONS" == 'true' ]]; then
        ARGS="$ARGS -e JOBS"
        COMMANDS="$BUILD_COMMANDS"
    else
        COMMANDS="$BUILD_COMMANDS"
    fi
    . $HELPERS_DIR/file-hash.sh $CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile
    COMMANDS="$PRE_COMMANDS && $COMMANDS"
    echo "$ docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\""
    eval docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\"
fi