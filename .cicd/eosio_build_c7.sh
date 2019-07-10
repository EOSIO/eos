#!/usr/bin/env bash
set -eu

rm -rf ./eos
docker build -t eos-dev-c7 -f centos-7.dockerfile .
git clone https://github.com/EOSIO/eos
cd eos
git checkout release/1.8.x
git submodule update --init --recursive

docker run --rm -v $(pwd):/eos eos-dev-c7 bash -c "source /opt/rh/devtoolset-8/enable && source /opt/rh/rh-python36/enable && mkdir /eos/build && cd /eos/build && cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DOPENSSL_ROOT_DIR='/usr/include/openssl' -DBUILD_MONGO_DB_PLUGIN=true /eos && make -j$(nproc)"

cd .. && rm -rf eos