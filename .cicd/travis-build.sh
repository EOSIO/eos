#!/bin/bash
set -e
CPU_CORES=$(getconf _NPROCESSORS_ONLN)
if [[ "$(uname)" == Darwin ]]; then
    echo 'Detected Darwin, building natively.'
    [[ -d eos ]] && cd eos
    [[ ! -d build ]] && mkdir build
    cd build
    echo '$ cmake ..'
    cmake ..
    echo "$ make -j $CPU_CORES"
    make -j $CPU_CORES
    echo 'Running unit tests.'
    echo "$ ctest -j $CPU_CORES -LE _tests --output-on-failure -T Test"
    ctest -j $CPU_CORES -LE _tests --output-on-failure -T Test # run unit tests
else # linux
    echo 'Detected Linux, building in Docker.'
    # docker
    docker run --rm -v $(pwd):/eos -v /usr/lib/ccache -v $HOME/.ccache:/opt/.ccache -e CCACHE_DIR=/opt/.ccache eosio/producer:ci-${IMAGE_TAG} bash -c " \
        $PRE_COMMANDS ccache -s && \
        mkdir /eos/build && cd /eos/build && $EXPORTS cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DOPENSSL_ROOT_DIR='/usr/include/openssl' -DBUILD_MONGO_DB_PLUGIN=true $CMAKE_EXTRAS /eos && make -j $(getconf _NPROCESSORS_ONLN) 
        ctest -j$(getconf _NPROCESSORS_ONLN) -LE _tests --output-on-failure -T Test"
fi