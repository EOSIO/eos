FROM ubuntu:18.04

ARG branch=master

RUN apt-get update \
    && apt-get install -y git sudo

RUN git clone -b $branch https://github.com/EOSIO/eos.git

WORKDIR /eos

RUN git submodule update --init --recursive

RUN echo 1 | ./eosio_build.sh

RUN cmake --build /eos/build --target install

COPY ./config.ini /

COPY ./nodeosd.sh /

RUN chmod +x /nodeosd.sh
