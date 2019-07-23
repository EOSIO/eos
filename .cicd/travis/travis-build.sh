#!/usr/bin/env bash
set -eo pipefail
ROOT_DIR=$(pwd)
cd $( dirname "${BASH_SOURCE[0]}" )/.. # Ensure we're in the .cicd dir
. ./.helpers
cd $ROOT_DIR
pwd

CPU_CORES=$(getconf _NPROCESSORS_ONLN)
if [[ "$(uname)" == Darwin ]]; then
    . ./.cicd/docker/.helpers-v33
    fold-execute ccache -s
    mkdir -p build && cd build && fold-execute cmake ..
    fold-execute make -j$(getconf _NPROCESSORS_ONLN)
    if ${ENABLE_PARALLEL_TESTS:-true}; then fold-execute ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test; fi
    if ${ENABLE_SERIAL_TESTS:-true}; then mkdir -p ./mongodb && fold-execute mongod --dbpath ./mongodb --fork --logpath mongod.log && fold-execute ctest -L nonparallelizable_tests --output-on-failure -T Test; fi
    if ${ENABLE_LR_TESTS:-false}; then fold-execute ctest -L long_running_tests --output-on-failure -T Test; fi
    if ${ENABLE_SUBMODULE_REGRESSION_TEST:-true}; then cd $ROOT_DIR && fold-execute ./.cicd/submodule-regression-checker.sh; fi
else # linux
    DOCKER_RUN_EXTRAS="-e ENABLE_PACKAGE_BUILDER=false" # Travis doesn't need to test or push packages
    execute eval docker run --rm -v $(pwd):/workdir -v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e CCACHE_DIR=/opt/.ccache -e TRAVIS $DOCKER_RUN_EXTRAS $FULL_TAG
fi