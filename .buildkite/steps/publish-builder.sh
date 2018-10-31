#!/bin/bash
set -euo pipefail

docker images
docker login -u=$DHUBU -p=$DHUBP
docker push cyberway/builder:${IMAGETAG}

if [[ "${IMAGETAG}" == "master" ]]; then
    docker tag cyberway/builder:${IMAGETAG} cyberway/builder:latest
    docker push cyberway/builder:latest
fi
