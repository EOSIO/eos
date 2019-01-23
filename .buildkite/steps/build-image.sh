#!/bin/bash
set -euo pipefail

IMAGETAG=${BUILDKITE_BRANCH:-master}
BRANCHNAME=${BUILDKITE_BRANCH:-master}

cd Docker
docker build -t cyberway/cyberway:${IMAGETAG} --build-arg=branch=${BRANCHNAME} --build-arg=builder=${BRANCHNAME} .
