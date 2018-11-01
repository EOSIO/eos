#!/bin/bash
set -euo pipefail

IMAGETAG=${BUILDKITE_BRANCH:-master}

cd Docker/builder
docker build -t cyberway/builder:${IMAGETAG} .