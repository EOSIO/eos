#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
mkdir -p $BUILD_DIR
CMAKE_EXTRAS="-DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON"
if [[ $(uname) == 'Darwin' ]]; then
    # You can't use chained commands in execute
    [[ $TRAVIS == true ]] && export PINNED=false && ccache -s && CMAKE_EXTRAS="-DCMAKE_CXX_COMPILER_LAUNCHER=ccache" && ./$CICD_DIR/platforms/macos-10.14.sh
    ( [[ ! $PINNED == false || $UNPINNED == true ]] ) && CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_TOOLCHAIN_FILE=$HELPERS_DIR/clang.make"
    cd $BUILD_DIR
    export SDKROOT=$(xcrun --show-sdk-path) # Fixes fatal error: 'string.h' file not found : https://github.com/conan-io/cmake-conan/issues/159#issuecomment-519486541
    cmake $CMAKE_EXTRAS ..
    make -j$JOBS
else # Linux
    ARGS=${ARGS:-"--rm --init -v $(pwd):$MOUNTED_DIR -e UNPINNED -e PINNED"}
    . $HELPERS_DIR/files-hash.sh $CICD_DIR/platforms/$IMAGE_TAG.dockerfile $ROOT_DIR/.conan/conanfile* # returns HASHED_IMAGE_TAG, etc
    PRE_COMMANDS="cd $MOUNTED_DIR/build"
    # PRE_COMMANDS: Executed pre-cmake
    # CMAKE_EXTRAS: Executed within and right before the cmake path (cmake CMAKE_EXTRAS ..)
    [[ ! $IMAGE_TAG =~ 'unpinned' ]] && CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_TOOLCHAIN_FILE=$MOUNTED_DIR/.cicd/helpers/clang.make -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    if [[ $IMAGE_TAG == 'amazon_linux-2' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib64/ccache:\\\$PATH"
    elif [[ $IMAGE_TAG == 'centos-7.6' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/rh-python36/enable && export PATH=/usr/lib64/ccache:\\\$PATH"
    elif [[ $IMAGE_TAG == 'ubuntu-16.04' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib/ccache:\\\$PATH"
    elif [[ $IMAGE_TAG == 'ubuntu-18.04' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib/ccache:\\\$PATH"
    elif [[ $IMAGE_TAG == 'amazon_linux-2-unpinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib64/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang'"
    elif [[ $IMAGE_TAG == 'centos-7.6-unpinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable && export PATH=/usr/lib64/ccache:\\\$PATH"
    elif [[ $IMAGE_TAG == 'ubuntu-18.04-unpinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang' -DLLVM_DIR='/usr/lib/llvm-7/lib/cmake/llvm'"
    fi
    BUILD_COMMANDS="cmake $CMAKE_EXTRAS .. && make -j$JOBS"
    # Docker Commands
    if [[ $BUILDKITE == true ]]; then
        # Generate Base Images
        $CICD_DIR/generate-base-images.sh
        [[ $ENABLE_INSTALL == true ]] && INSTALL_COMMANDS="&& cp -r $MOUNTED_DIR /root/eosio && cd /root/eosio/build && make install" # cp into /root/eosio prevents make install from failing and also helps keep it in the container for contracts to use
    elif [[ $TRAVIS == true ]]; then
        ARGS="$ARGS -v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e JOBS -e TRAVIS -e CCACHE_DIR=/opt/.ccache"
        COMMANDS="ccache -s &&"
    fi
    COMMANDS="$PRE_COMMANDS && $COMMANDS $BUILD_COMMANDS $INSTALL_COMMANDS"
    if [[ $DOCKER == true ]]; then # Only run commands when we're already in docker (base image creation)
        echo $COMMANDS
        eval bash -c \"$COMMANDS\"
    else
        echo "$ docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\""
        eval docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\"
    fi
fi