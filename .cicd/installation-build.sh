#!/usr/bin/env bash
set -eo pipefail
. ./.helpers

./.cicd/generate-base-images.sh

docker run -v $(pwd):/workdir -e JOBS -e $FULL_TAG
