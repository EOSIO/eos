FROM cyberway/builder:latest as builder
ARG branch=master
ARG symbol=CYBER

RUN git clone -b $branch https://github.com/GolosChain/cyberway.git --recursive \
    && cd cyberway \
    && echo "$branch:$(git rev-parse HEAD)" > /etc/cyberway-version \
    && cmake -H. -B"/tmp/build" \
        -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DWASM_ROOT=/opt/wasm \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_INSTALL_PREFIX=/tmp/build \
        -DBUILD_MONGO_DB_PLUGIN=false \
        -DCORE_SYMBOL_NAME=$symbol \
    && cmake --build /tmp/build --target install \
    && rm /tmp/build/bin/eosiocpp

FROM ubuntu:18.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /tmp/build/bin /opt/cyberway/bin
COPY --from=builder /tmp/build/contracts /contracts
COPY --from=builder /cyberway/Docker/config.ini /
COPY --from=builder /etc/cyberway-version /etc
COPY --from=builder /cyberway/Docker/nodeosd.sh /opt/cyberway/bin/nodeosd.sh
COPY --from=builder /usr/local/eosio.cdt/* /usr/local/cyberway.cdt/

ENV CYBERWAY_ROOT=/opt/cyberway
RUN chmod +x /opt/cyberway/bin/nodeosd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /opt/cyberway/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
