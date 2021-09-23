#!/bin/bash
set -euo pipefail

. "${0%/*}/libfunctions.sh"

echo '--- :docker: Pretest Setup'

perform "docker pull $IMAGE"
perform "docker run --rm -v \"\$(pwd):/eos\" -w '/eos' -it $IMAGE ./.cicd/test-package.run.sh"
