#!/usr/bin/env bash
set -eo pipefail
cd $( dirname "${BASH_SOURCE[0]}" )/.. # Ensure we're in the repo root and not inside of scripts
. ./.cicd/.helpers

CPU_CORES=$(getconf _NPROCESSORS_ONLN)
if [[ "$(uname)" == Darwin ]]; then
    echo 'Detected Darwin, building natively.'
    [[ -d eos ]] && cd eos
    [[ ! -d build ]] && mkdir build
    cd build
    echo \$PATH
    ccache -s
    echo '$ cmake ..'
    cmake ..
    echo "$ make -j$JOBS"
    make -j$JOBS
    echo 'Running unit tests.'
    echo "$ ctest -j$JOBS -LE _tests --output-on-failure -T Test"
    ctest -j$JOBS -LE _tests --output-on-failure -T Test # run unit tests
else # linux
    execute docker run --rm -v $(pwd):/workdir -v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e JOBS -e CCACHE_DIR=/opt/.ccache $FULL_TAG
fi