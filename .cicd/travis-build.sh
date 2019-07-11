#!/bin/bash
set -e
CPU_CORES=$(getconf _NPROCESSORS_ONLN)
if [[ "$(uname)" == Darwin ]]; then
    echo 'Detected Darwin, building natively.'
    [[ -d eos ]] && cd eos
    [[ ! -d build ]] && mkdir build
    cd build
    cmake ..
    make -j $CPU_CORES
else # linux
    echo 'Detected Linux, building in Docker.'
    echo "$ docker pull eosio/producer:ci-$IMAGE_TAG"
    docker pull eosio/producer:ci-$IMAGE_TAG
    echo "docker run --rm -v $(pwd):/eos eosio/producer:ci-$IMAGE_TAG bash -c \"mkdir /eos/build && cd /eos/build && cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DOPENSSL_ROOT_DIR='/usr/include/openssl' -DBUILD_MONGO_DB_PLUGIN=true /eos && make -j $(getconf _NPROCESSORS_ONLN)\""
    docker run --rm -v $(pwd):/eos -v $HOME/.ccache:/usr/lib/ccache -e CCACHE_DIR=/usr/lib/ccache eosio/producer:ci-$IMAGE_TAG bash -c " \
    export PATH=/usr/lib/ccache:\$PATH && apt-get update; apt-get install -y ccache && ccache -s; \
    mkdir /eos/build && cd /eos/build && cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DOPENSSL_ROOT_DIR='/usr/include/openssl' -DBUILD_MONGO_DB_PLUGIN=true /eos && make -j $(getconf _NPROCESSORS_ONLN)"
fi