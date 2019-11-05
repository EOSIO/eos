#!/bin/bash
set -eo pipefail
. ./.cicd/helpers/general.sh
mkdir -p $BUILD_DIR
CMAKE_EXTRAS="-DCMAKE_BUILD_TYPE='Release'"
if [[ "$(uname)" == 'Darwin' ]]; then
    # You can't use chained commands in execute
    if [[ "$TRAVIS" == 'true' ]]; then
        export PINNED=false
        ccache -s
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    else
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DBUILD_MONGO_DB_PLUGIN=true"
    fi
    [[ ! "$PINNED" == 'false' ]] && CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_TOOLCHAIN_FILE=$HELPERS_DIR/clang.make"
    cd $BUILD_DIR
    echo "cmake $CMAKE_EXTRAS .."
    cmake $CMAKE_EXTRAS ..
    echo "make -j$JOBS"
    make -j$JOBS
else # Linux
    CMAKE_EXTRAS="$CMAKE_EXTRAS -DBUILD_MONGO_DB_PLUGIN=true"
    ARGS=${ARGS:-"--rm --init -v $(pwd):$MOUNTED_DIR"}
    PRE_COMMANDS="cd $MOUNTED_DIR/build"
    # PRE_COMMANDS: Executed pre-cmake
    # CMAKE_EXTRAS: Executed within and right before the cmake path (cmake CMAKE_EXTRAS ..)
    [[ ! "$IMAGE_TAG" =~ 'unpinned' ]] && CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_TOOLCHAIN_FILE=$MOUNTED_DIR/.cicd/helpers/clang.make -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    if [[ $IMAGE_TAG == 'amazon_linux-2-pinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib64/ccache:\\\$PATH"
    elif [[ "$IMAGE_TAG" == 'centos-7.6-pinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib64/ccache:\\\$PATH"
    elif [[ "$IMAGE_TAG" == 'ubuntu-16.04-pinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib/ccache:\\\$PATH"
    elif [[ "$IMAGE_TAG" == 'ubuntu-18.04-pinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib/ccache:\\\$PATH"
    elif [[ "$IMAGE_TAG" == 'amazon_linux-2-unpinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib64/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang'"
    elif [[ "$IMAGE_TAG" == 'centos-7.6-unpinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable && export PATH=/usr/lib64/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DLLVM_DIR='/opt/rh/llvm-toolset-7.0/root/usr/lib64/cmake/llvm'"
    elif [[ "$IMAGE_TAG" == 'ubuntu-18.04-unpinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++-7' -DCMAKE_C_COMPILER='clang-7' -DLLVM_DIR='/usr/lib/llvm-7/lib/cmake/llvm'"
    fi
    BUILD_COMMANDS="cmake $CMAKE_EXTRAS .. && make -j$JOBS"
    # Docker Commands
    if [[ "$BUILDKITE" == 'true' ]]; then
        # Generate Base Images
        $CICD_DIR/generate-base-images.sh
        [[ "$ENABLE_INSTALL" == 'true' ]] && COMMANDS="cp -r $MOUNTED_DIR /root/eosio && cd /root/eosio/build &&"
        COMMANDS="$COMMANDS $BUILD_COMMANDS"
        [[ "$ENABLE_INSTALL" == 'true' ]] && COMMANDS="$COMMANDS && make install"
    elif [[ "$TRAVIS" == 'true' ]]; then
        ARGS="$ARGS -v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e JOBS -e TRAVIS -e CCACHE_DIR=/opt/.ccache"
        COMMANDS="ccache -s && $BUILD_COMMANDS"
    fi
    . $HELPERS_DIR/file-hash.sh $CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile
    COMMANDS="$PRE_COMMANDS && $COMMANDS"
    echo "$ docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\""
    eval docker run $ARGS $(buildkite-intrinsics) $FULL_TAG bash -c \"$COMMANDS\"
fi