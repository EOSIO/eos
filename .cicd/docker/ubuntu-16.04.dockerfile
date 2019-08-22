FROM ubuntu:16.04
# install dependencies.
RUN apt-get update && apt-get upgrade -y && apt install -y git sudo jq

ENV VERBOSE true

# build cmake.
RUN git clone https://github.com/EOSIO/eos.git -b release/1.7.x \
    && cd eos \
    && git submodule update --init --recursive \
    && ./scripts/eosio_build.sh -y \
    && cd .. && rm -rf eos

RUN mkdir -p ~/.ssh && chmod 700 ~/.ssh && ssh-keyscan -H github.com >> ~/.ssh/known_hosts