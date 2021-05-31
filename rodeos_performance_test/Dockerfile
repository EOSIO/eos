FROM ubuntu:20.04
RUN apt-get update && \
    apt-get upgrade -y && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y libusb-1.0-0 curl gnupg netcat libcurl3-gnutls jq && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
ENV TZ=America/Chicago
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone && \
    echo "deb http://apt.postgresql.org/pub/repos/apt focal-pgdg main" > /etc/apt/sources.list.d/pgdg.list && \
    curl -sL https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - && \
    apt-get update && apt-get -y install libpq5 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
COPY contracts /root/eosio/contracts
COPY boot.sh /root/eosio/boot.sh
COPY wait-for-it.sh /root/eosio/wait-for-it.sh
COPY config.ini /etc/eosio/config.ini
COPY rocksdb.ini /etc/rodeos/rocksdb.ini
ARG BUILD_DIR
COPY unittests $BUILD_DIR/unittests
WORKDIR /root/eosio
COPY bin /usr/local/bin

