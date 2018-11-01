#!/bin/bash
set -euo pipefail

IMAGETAG=${BUILDKITE_BRANCH:-master}
BRANCHNAME=${BUILDKITE_BRANCH:-master}

docker images

docker login -u=$DHUBU -p=$DHUBP
docker push cyberway/cyberway:${IMAGETAG}

if [[ "${IMAGETAG}" == "master" ]]; then
    docker tag cyberway/cyberway:${IMAGETAG} cyberway/cyberway:latest
    docker push cyberway/cyberway:latest
fi

