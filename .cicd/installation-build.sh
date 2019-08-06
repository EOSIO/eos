#!/usr/bin/env bash
set -eo pipefail
. ./.helpers

execute docker run -v $(pwd):/workdir -e JOBS -e ENABLE_INSTALL=true -e $FULL_TAG
