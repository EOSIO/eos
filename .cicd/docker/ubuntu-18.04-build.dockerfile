FROM eosio/producer:eosio-ubuntu-18.04-2049ab48a02aee3a7c8964291408259eed4bbc8b
COPY . /eos
ENV CCACHE_DIR=/opt/.ccache
ENV CMAKE_FRAMEWORK_PATH='/usr/local'
ENV ENABLE_PARALLEL_TESTS=false
ENV ENABLE_SERIAL_TESTS=false
ENV ENABLE_INSTALL=true
ENV BUILDKITE_AGENT_ACCESS_TOKEN=$BUILDKITE_AGENT_ACCESS_TOKEN
RUN bash -c '[[ -d /eos/build ]] || mkdir /eos/build'
RUN bash -c 'cd /eos/build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCORE_SYMBOL_NAME=SYS -DOPENSSL_ROOT_DIR=/usr/include/openssl -DBUILD_MONGO_DB_PLUGIN=true .. && \
    make -j $(nproc) && \
    make install'
RUN bash -c 'cd /eos && \
    tar -pczf build.tar.gz build && \
    buildkite-agent artifact upload build.tar.gz'