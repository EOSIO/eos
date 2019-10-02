FROM ubuntu:18.04
ENV VERSION 1
ENV PATH="/usr/local/cmake/bin:${PATH}"

RUN apt-get update && apt-get upgrade -y
