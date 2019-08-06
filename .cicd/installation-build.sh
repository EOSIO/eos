#!/usr/bin/env bash
set -eo pipefail
. ./.helpers

cd ..
execute docker run -v $(pwd):/workdir -e JOBS -e ENABLE_INSTALL=true $FULL_TAG
