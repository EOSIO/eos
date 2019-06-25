ARG builder=master
FROM cyberway/builder:$builder as builder
ARG branch=master
ARG symbol=CYBER
ARG compiletype=RelWithDebInfo

ADD https://api.github.com/repos/GolosChain/cyberway/git/refs/heads/$branch /etc/version.json

RUN git clone -b $branch https://github.com/GolosChain/cyberway.git --recursive

RUN cd cyberway \
    && echo "$branch:$(git rev-parse HEAD)" > /etc/cyberway-version \
    && cmake -H. -B"/tmp/build" \
        -GNinja \
        -DCMAKE_BUILD_TYPE=$compiletype \
        -DWASM_ROOT=/opt/wasm \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_INSTALL_PREFIX=/opt/cyberway \
        -DBUILD_MONGO_DB_PLUGIN=false \
        -DCORE_SYMBOL_NAME=$symbol \
    && cmake --build /tmp/build --target install

FROM ubuntu:18.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates libusb-1.0-0-dev libcurl4-gnutls-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /cyberway/Docker/config.ini /
COPY --from=builder /etc/cyberway-version /etc
COPY --from=builder /opt/cyberway /opt/cyberway
COPY --from=builder /cyberway/Docker/nodeosd.sh /opt/cyberway/bin/nodeosd.sh
COPY --from=builder /cyberway/Docker/run-with-events.sh /opt/cyberway/startup/run-with-events.sh
COPY --from=builder /cyberway/Docker/docker-compose-events.yml /opt/cyberway/startup/docker-compose-events.yml
COPY --from=builder /cyberway/Docker/mongodb-exporter /opt/cyberway/startup/mongodb-exporter
COPY --from=builder /cyberway/Docker/nats /opt/cyberway/startup/nats

ENV CYBERWAY_ROOT=/opt/cyberway
RUN chmod +x /opt/cyberway/bin/nodeosd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /opt/cyberway/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
