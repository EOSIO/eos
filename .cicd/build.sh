#!/bin/bash
set -eo pipefail
[[ "$ENABLE_INSTALL" == 'true' ]] || echo '--- :evergreen_tree: Configuring Environment'
. ./.cicd/helpers/general.sh
mkdir -p "$BUILD_DIR"
CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_BUILD_TYPE=\"Release\" -DENABLE_MULTIVERSION_PROTOCOL_TEST=\"true\" -DAMQP_CONN_STR=\"amqp://guest:guest@localhost:5672\""
if [[ "$(uname)" == 'Darwin' && "$FORCE_LINUX" != 'true' ]]; then
    # You can't use chained commands in execute
    if [[ "$GITHUB_ACTIONS" == 'true' ]]; then
        export PINNED='false'
    fi
    [[ ! "$PINNED" == 'false' ]] && CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_TOOLCHAIN_FILE=\"$HELPERS_DIR/clang.make\""
    cd "$BUILD_DIR"
    [[ "$CI" == 'true' ]] && source ~/.bash_profile # Make sure node is available for ship_test
    echo '+++ :hammer_and_wrench: Building EOSIO'
    CMAKE_COMMAND="cmake $CMAKE_EXTRAS .."
    echo "$ $CMAKE_COMMAND"
    eval $CMAKE_COMMAND
    MAKE_COMMAND="make -j '$JOBS'"
    echo "$ $MAKE_COMMAND"
    eval $MAKE_COMMAND
    cd ..
else # Linux
    ARGS=${ARGS:-"--rm --init -v \"\$(pwd):$MOUNTED_DIR\""}
    PRE_COMMANDS="cd \"$MOUNTED_DIR/build\""
    # PRE_COMMANDS: Executed pre-cmake
    # CMAKE_EXTRAS: Executed within and right before the cmake path (cmake CMAKE_EXTRAS ..)
    [[ ! "$IMAGE_TAG" =~ 'unpinned' ]] && CMAKE_EXTRAS="$CMAKE_EXTRAS -DTPM2TSS_STATIC=\"On\" -DCMAKE_TOOLCHAIN_FILE=\"$MOUNTED_DIR/.cicd/helpers/clang.make\""
    if [[ "$IMAGE_TAG" == 'amazon_linux-2-unpinned' ]]; then
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER=\"clang++\" -DCMAKE_C_COMPILER=\"clang\""
    elif [[ "$IMAGE_TAG" == 'centos-7.7-unpinned' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/devtoolset-8/enable"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DLLVM_DIR=\"/opt/rh/llvm-toolset-7.0/root/usr/lib64/cmake/llvm\""
    elif [[ "$IMAGE_TAG" == 'ubuntu-18.04-unpinned' ]]; then
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER=\"clang++-7\" -DCMAKE_C_COMPILER=\"clang-7\" -DLLVM_DIR=\"/usr/lib/llvm-7/lib/cmake/llvm\""
    fi
    if [[ "$IMAGE_TAG" == centos-7.* ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/rh-python36/enable"
    fi
    CMAKE_COMMAND="cmake \$CMAKE_EXTRAS .."
    MAKE_COMMAND="make -j $JOBS"
    BUILD_COMMANDS="echo \"+++ :hammer_and_wrench: Building EOSIO\" && echo \"$ $CMAKE_COMMAND\" && eval $CMAKE_COMMAND && echo \"$ $MAKE_COMMAND\" && eval $MAKE_COMMAND"
    # Docker Commands
    if [[ "$BUILDKITE" == 'true' ]]; then
        # Generate Base Images
        BASE_IMAGE_COMMAND="\"$CICD_DIR/generate-base-images.sh\""
        echo "$ $BASE_IMAGE_COMMAND"
        eval $BASE_IMAGE_COMMAND
        [[ "$ENABLE_INSTALL" == 'true' ]] && COMMANDS="cp -r \"$MOUNTED_DIR\" \"/root/eosio\" && cd \"/root/eosio/build\" &&"
        COMMANDS="$COMMANDS $BUILD_COMMANDS"
        [[ "$ENABLE_INSTALL" == 'true' ]] && COMMANDS="$COMMANDS && make install"
    elif [[ "$GITHUB_ACTIONS" == 'true' ]]; then
        ARGS="$ARGS -e JOBS"
        COMMANDS="$BUILD_COMMANDS"
    else
        COMMANDS="$BUILD_COMMANDS"
    fi
    . "$HELPERS_DIR/file-hash.sh" "$CICD_DIR/platforms/$PLATFORM_TYPE/$IMAGE_TAG.dockerfile"
    COMMANDS="$PRE_COMMANDS && $COMMANDS"
    DOCKER_RUN="docker run $ARGS $(buildkite-intrinsics) --env CMAKE_EXTRAS='$CMAKE_EXTRAS' '$FULL_TAG' bash -c '$COMMANDS'"
    echo "$ $DOCKER_RUN"
    eval $DOCKER_RUN
fi
if [[ "$BUILDKITE" == 'true' && "$ENABLE_INSTALL" != 'true' ]]; then
    echo '--- :arrow_up: Uploading Artifacts'
    echo 'Compressing build directory.'
    tar -pczf 'build.tar.gz' build
    echo 'Uploading build directory.'
    buildkite-agent artifact upload 'build.tar.gz'
    echo 'Done uploading artifacts.'
fi
[[ "$ENABLE_INSTALL" == 'true' ]] || echo '--- :white_check_mark: Done!'
