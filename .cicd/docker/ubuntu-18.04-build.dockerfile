FROM eosio/producer:eosio-ubuntu-18.04-d1248048f40158965ff9876c0c21ac728bb82da6
COPY . /eos
ENV CCACHE_DIR=/opt/.ccache
ENV CMAKE_FRAMEWORK_PATH='/usr/local'
ENV ENABLE_PARALLEL_TESTS=false
ENV ENABLE_SERIAL_TESTS=false
ENV ENABLE_INSTALL=true
RUN bash -c "[[ -d /eos/build ]] || mkdir /eos/build ; \
    cd /eos/build && \
    cmake -DCMAKE_BUILD_TYPE='Release' -DCORE_SYMBOL_NAME='SYS' -DOPENSSL_ROOT_DIR='/usr/include/openssl' -DBUILD_MONGO_DB_PLUGIN=true .. && \
    make -j $(nproc) && \
    make install && \
    cd .. && \
    echo $(pkill mongod || :) && \
    tar -pczf build.tar.gz build"
COPY /eos/build.tar.gz .