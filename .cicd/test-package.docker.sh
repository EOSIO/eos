#!/bin/bash
set -euo pipefail

. "${0%/*}/helpers/perform.sh"

echo '--- :docker: Pretest Setup'

perform "docker pull $IMAGE"
DOCKER_RUN_ARGS="--rm -v \"\$(pwd):/eos\" -w '/eos' -it $IMAGE ./.cicd/test-package.run.sh"
echo "$ docker run $DOCKER_RUN_ARGS"
[[ -z "${PROXY_DOCKER_BUILD_ARGS:-}" ]] || echo "Appending proxy args: '${PROXY_DOCKER_BUILD_ARGS}'"
eval "docker run ${PROXY_DOCKER_RUN_ARGS}${DOCKER_RUN_ARGS}"
