FROM golang:latest as builder

WORKDIR /go/src/github.com/percona/mongodb_exporter

ARG tag=v0.6.2

RUN \
    apt-get update \
    && apt-get install -y wget git ca-certificates

RUN \
    git clone -b $tag --recursive https://github.com/percona/mongodb_exporter.git . \
    && make build

FROM quay.io/prometheus/busybox:latest

COPY --from=builder /go/src/github.com/percona/mongodb_exporter/mongodb_exporter /bin/

EXPOSE      9216
ENTRYPOINT  [ "/bin/mongodb_exporter" ]
