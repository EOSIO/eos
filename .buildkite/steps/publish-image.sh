#!/bin/bash
set -euo pipefail

docker images

IMAGETAG=${BUILDKITE_BRANCH:-master}
BRANCHNAME=${BUILDKITE_BRANCH:-master}

docker login -u=$DHUBU -p=$DHUBP
docker push cyberway/cyberway:${IMAGETAG}

if [[ "${IMAGETAG}" == "master" ]]; then
    docker tag cyberway/cyberway:${IMAGETAG} cyberway/cyberway:latest
    docker push cyberway/cyberway:latest
fi

