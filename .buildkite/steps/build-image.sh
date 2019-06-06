#!/bin/bash
set -euo pipefail

IMAGETAG=${BUILDKITE_BRANCH:-master}
BRANCHNAME=${BUILDKITE_BRANCH:-master}
COMPILETYPE=RelWithDebInfo

if [[ "${IMAGETAG}" == "master" ]]; then
    COMPILETYPE=Release
fi

cd Docker
docker build -t cyberway/cyberway:${IMAGETAG} --build-arg=branch=${BRANCHNAME} --build-arg=builder=${BRANCHNAME} --build-arg=compiletype=${COMPILETYPE} .
