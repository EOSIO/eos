FROM ubuntu:18.04

COPY *.deb .

RUN apt update && \
  apt install -y curl wget && \
  apt install -y /*.deb && \
  rm -rf /*.deb /var/lib/apt/lists/*