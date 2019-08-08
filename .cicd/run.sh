#!/usr/bin/env bash
set -eo pipefail
. ./.cicd/helpers/general.sh
. ./$HELPERS_DIR/execute.sh

execute mkdir -p $BUILD_DIR

PARALLEL_TEST_COMMAND="fold-execute ctest -j$JOBS -LE _tests --output-on-failure -T Test"
SERIAL_TEST_COMMAND="mkdir -p ./mongodb && fold-execute mongod --dbpath ./mongodb --fork --logpath mongod.log && fold-execute ctest -L nonparallelizable_tests --output-on-failure -T Test"
LONG_RUNNING_TEST_COMMAND="fold-execute ctest -L long_running_tests --output-on-failure -T Test"

if [[ $(uname) == 'Darwin' ]]; then

    . $HELPERS_DIR/logging.sh

    # You can't use chained commands in execute
    MAC_CMAKE="fold-execute cmake .."
    MAC_MAKE="fold-execute make -j$JOBS"
    fold-execute ccache -s
    fold-execute cd $BUILD_DIR
    if [[ $ENABLE_BUILD == true ]] || [[ $TRAVIS == true ]]; then
        $MAC_CMAKE
        $MAC_MAKE
    fi
    if [[ $BUILDKITE == true ]]; then
        [[ $ENABLE_PARALLEL_TESTS == true ]] && $PARALLEL_TEST_COMMAND || true
        [[ $ENABLE_SERIAL_TESTS == true ]] && $SERIAL_TEST_COMMAND || true
        [[ $ENABLE_LONG_RUNNING_TESTS == true ]] && $LONG_RUNNING_TEST_COMMAND || true
    elif [[ $TRAVIS == true ]]; then
        $PARALLEL_TEST_COMMAND
        $SERIAL_TEST_COMMAND
    fi

else # Linux

    MOUNTED_DIR='/workdir'
    ARGS=${ARGS:-"--rm -v $(pwd):$MOUNTED_DIR"}

    function append-to-commands() { # Useful to avoid having to specify && between commands we need in the final COMMANDS var we use in Docker Run
        [[ ! -z $COMMANDS ]] && export COMMANDS="$COMMANDS && $@" || export COMMANDS="$@"
    }

    . ./$HELPERS_DIR/docker-hash.sh

    PRE_COMMANDS=". $MOUNTED_DIR/.cicd/helpers/logging.sh && fold-execute ccache -s && cd $MOUNTED_DIR/build"
    # PRE_COMMANDS: Executed pre-cmake
    # CMAKE_EXTRAS: Executed within and right before the cmake path (cmake CMAKE_EXTRAS ..)
    if [[ $IMAGE_TAG == 'ubuntu-18.04' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang'"
    elif [[ $IMAGE_TAG == 'ubuntu-16.04' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_TOOLCHAIN_FILE=$MOUNTED_DIR/.cicd/helpers/clang.make -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    elif [[ $IMAGE_TAG == 'centos-7.6' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable && export PATH=/usr/lib64/ccache:\\\$PATH"
    elif [[ $IMAGE_TAG == 'amazonlinux-2' ]]; then
        PRE_COMMANDS="$PRE_COMMANDS && export PATH=/usr/lib64/ccache:\\\$PATH"
        CMAKE_EXTRAS="$CMAKE_EXTRAS -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_C_COMPILER='clang'"
    fi

    MOVE_REPO_COMMANDS="fold-execute cp -r $MOUNTED_DIR ~/eosio && fold-execute cd ~/eosio/build"
    BUILD_COMMANDS="fold-execute cmake $CMAKE_EXTRAS .. && fold-execute make -j$JOBS"
    INSTALL_COMMANDS="fold-execute make install"

    append-to-commands $PRE_COMMANDS

    # Docker Commands
    if [[ $BUILDKITE == true ]]; then
        # Generate Base Images
        execute ./.cicd/generate-base-images.sh
        [[ $ENABLE_INSTALL == true ]] && append-to-commands $MOVE_REPO_COMMANDS
        [[ $ENABLE_BUILD == true ]] && append-to-commands $BUILD_COMMANDS
        [[ $ENABLE_INSTALL == true ]] && append-to-commands $INSTALL_COMMANDS
        [[ $ENABLE_PARALLEL_TESTS == true ]] && append-to-commands $PARALLEL_TEST_COMMAND
        [[ $ENABLE_SERIAL_TESTS == true ]] && append-to-commands $SERIAL_TEST_COMMAND
        [[ $ENABLE_LONG_RUNNING_TESTS == true ]] && append-to-commands $LONG_RUNNING_TEST_COMMAND
    elif [[ $TRAVIS == true ]]; then
        ARGS="$ARGS -v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e JOBS -e TRAVIS -e CCACHE_DIR=/opt/.ccache"
        COMMANDS="$PRE_COMMANDS && $BUILD_COMMANDS && $PARALLEL_TEST_COMMAND && $SERIAL_TEST_COMMAND"
    fi

    # Load BUILDKITE Environment Variables for use in docker run
    if [[ -f $BUILDKITE_ENV_FILE ]]; then
        evars=""
        while read -r var; do
            evars="$evars --env ${var%%=*}"
        done < "$BUILDKITE_ENV_FILE"
    fi
    execute eval docker run $ARGS $evars $FULL_TAG bash -c \"$COMMANDS\"

fi