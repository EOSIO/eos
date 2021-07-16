#!/bin/bash
set -eu

echo '--- :docker: Selecting Container'

docker pull $IMAGE
docker run --rm -v "$(pwd):/eos" -w '/eos' -it $IMAGE ./.cicd/test-package.run.sh
