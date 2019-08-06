#!/usr/bin/env bash
set -eo pipefail
. ./.helpers

execute docker run -v $(pwd):/workdir -e JOBS -e ENABLE_INSTALL=true $FULL_TAG bash -c "pwd && ls -la /workdir"
