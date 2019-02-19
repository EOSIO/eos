#!/bin/bash

set -euo pipefail

TAG=v0.6.2

cd Docker/mongodb-exporter
docker build -t cyberway/mongodb-exporter:${TAG} --build-arg="tag=${TAG}" .

docker login -u=$DHUBU -p=$DHUBP
docker push cyberway/mongodb-exporter:${TAG}
