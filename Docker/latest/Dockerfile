FROM eosio/builder as builder

ARG GITHUB_BRANCH=master
ARG GITHUB_ORG=EOSIO
ARG DOCKERHUB_USERNAME=eosio
ARG IMAGE_NAME=eos
ARG IMAGE_TAG=latest
ARG BUILD_SYMBOL=EOS
ARG NODEOS_VOLUME_NAME=nodeos-data-volume
ARG KEOSD_VOLUME_NAME=keosd-data-volume
ARG COMPOSE_PROJECT_NAME=main

RUN git clone -b $GITHUB_BRANCH https://github.com/$GITHUB_ORG/eos.git --recursive \
    && cd eos && echo "$GITHUB_BRANCH:$(git rev-parse HEAD)" > /etc/eosio-version \
    && cmake -H. -B"/tmp/build" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_ROOT=/opt/wasm -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/tmp/build  -DSecp256k1_ROOT_DIR=/usr/local -DBUILD_MONGO_DB_PLUGIN=true -DCORE_SYMBOL_NAME=$BUILD_SYMBOL \
    && cmake --build /tmp/build --target install && rm /tmp/build/bin/eosiocpp

FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /tmp/build/bin /opt/eosio/bin
COPY --from=builder /tmp/build/contracts /contracts
COPY --from=builder /eos/Docker/config.ini /
COPY --from=builder /etc/eosio-version /etc
COPY --from=builder /eos/Docker/nodeosd.sh /opt/eosio/bin/nodeosd.sh
ENV EOSIO_ROOT=/opt/eosio
RUN chmod +x /opt/eosio/bin/nodeosd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
VOLUME /opt/eosio/bin/data-dir
ENV PATH /opt/eosio/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
