FROM ubuntu:16.04
RUN apt-get update && apt-get upgrade -y && apt install -y git sudo jq

ENV VERBOSE true
RUN git clone https://github.com/EOSIO/eos.git -b release/1.7.x \
    && cd eos \
    && git submodule update --init --recursive \
    && ./scripts/eosio_build.sh -y -m \
    && cd .. && rm -rf eos

RUN mkdir -p ~/.ssh && chmod 700 ~/.ssh && ssh-keyscan -H github.com >> ~/.ssh/known_hosts

ENV PATH=/root/bin:${PATH}:/root/opt/mongodb/bin
ENV BOOST_ROOT=/root/opt/boost