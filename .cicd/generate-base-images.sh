#!/usr/bin/env bash
set -eo pipefail
cd $( dirname "${BASH_SOURCE[0]}" )/.. # Ensure we're in the repo root and not inside of scripts
. ./.cicd/.helpers
echo "Looking for $FULL_TAG"
docker_tag_exists $FULL_TAG && echo "$FULL_TAG already exists" || generate_docker_image